"""Build a C++ scheme and inspect the Chromium surfaces it actually displays."""
import argparse
import base64
import json
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory
import time
from urllib.request import urlopen

import websocket


SCHEME = """Make(cpp)
Add(MainForm,1,0,0)
{
 Width=980
 Height=660
 BuildMode=1
 link(onCreate,3:doPlay,[])
}
Add(WebBrowser,2,100,60)
{
 Left=8
 Top=8
 Width=940
 Height=90
 URL=page.html
}
Add(Flash,3,160,80)
{
 Left=8
 Top=106
 Width=940
 Height=520
 FileName=movie.swf
}
"""


def sample_movie():
    """One SWF frame with a 200 by 100 pixel stage and a background color."""
    rectangle = "01110" + "".join(f"{coordinate:014b}" for coordinate in (0, 4000, 0, 2000))
    rectangle = rectangle.ljust(64, "0")
    frame = int(rectangle, 2).to_bytes(8, "big") + struct.pack("<HH", 24 * 256, 1)
    frame += struct.pack("<H", 9 * 64 + 3) + b"\x20\x79\xae"  # SetBackgroundColor
    frame += struct.pack("<HH", 64, 0)  # ShowFrame and End
    return b"FWS\x06" + struct.pack("<I", 8 + len(frame)) + frame


def until(check, description, timeout=60):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            if result := check():
                return result
        except (OSError, websocket.WebSocketException):
            pass
        time.sleep(0.25)
    raise AssertionError(f"Timed out waiting for {description}")


class Page:
    def __init__(self, debugger_url):
        self.connection = websocket.create_connection(debugger_url, timeout=6, suppress_origin=True)
        self.sequence = 0

    def call(self, method, **params):
        self.sequence += 1
        self.connection.send(json.dumps({"id": self.sequence, "method": method, "params": params}))
        while True:
            reply = json.loads(self.connection.recv())
            if reply.get("id") == self.sequence:
                if "error" in reply:
                    raise AssertionError(reply["error"])
                return reply["result"]

    def evaluate(self, expression):
        result = self.call("Runtime.evaluate", expression=expression, returnByValue=True)
        if "exceptionDetails" in result:
            raise AssertionError(result["exceptionDetails"])
        return result["result"].get("value")

    def screenshot(self):
        png = base64.b64decode(self.call("Page.captureScreenshot", format="png")["data"])
        assert png.startswith(b"\x89PNG\r\n\x1a\n")
        width, height = struct.unpack_from(">II", png, 16)
        assert width > 100 and height > 20, (width, height)
        return png

    def close(self):
        self.connection.close()


def verify(app, movie, artifacts):
    # On Windows the browser subprocess may release its profile just after its Qt host exits.
    with TemporaryDirectory(prefix="hiasm-web-widgets-", ignore_cleanup_errors=True) as directory:
        workspace = Path(directory)
        scheme = workspace / "WebWidgets.sha"
        scheme.write_text(SCHEME, encoding="utf-8")
        (workspace / "page.html").write_text(
            '<title>HiAsm Browser</title><p>Chromium ready</p><a id="next" href="next.html">Next</a>',
            encoding="utf-8",
        )
        (workspace / "next.html").write_text("<title>Next page</title><p>Followed link</p>", encoding="utf-8")
        if movie:
            shutil.copyfile(movie, workspace / "movie.swf")
        else:
            (workspace / "movie.swf").write_bytes(sample_movie())
        output = workspace / "run"
        output.mkdir()
        executable = output / ("web-widgets.exe" if os.name == "nt" else "web-widgets")
        result = subprocess.run([app, "--build", scheme, executable], cwd=workspace,
                                capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=240)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            port = listener.getsockname()[1]
        env = os.environ.copy()
        env.pop("QT_PLUGIN_PATH", None)
        if os.name == "nt":
            env["QT_QPA_PLATFORM"] = "windows"
            env["WEBVIEW2_USER_DATA_FOLDER"] = str(workspace / "edge-profile")
            env["WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS"] = f"--remote-debugging-port={port}"
        else:
            env["QT_QPA_PLATFORM"] = "cocoa" if sys.platform == "darwin" else "xcb"
            env["QTWEBENGINE_REMOTE_DEBUGGING"] = f"127.0.0.1:{port}"
        log_path = workspace / "runtime.log"
        browser = flash = None
        with log_path.open("wb") as log:
            process = subprocess.Popen([executable], cwd=workspace, env=env, stdout=log, stderr=log)
            try:
                def targets_ready():
                    if process.poll() is not None:
                        raise AssertionError(f"Application exited with code {process.returncode}")
                    with urlopen(f"http://127.0.0.1:{port}/json/list", timeout=2) as response:
                        targets = json.load(response)
                    web_page = next((page for page in targets if page["title"] == "HiAsm Browser"), None)
                    flash_page = next((page for page in targets if page["url"].endswith("/ruffle_player.html")), None)
                    return (web_page, flash_page) if web_page and flash_page else None

                web_page, flash_page = until(targets_ready, "both Chromium views")
                browser = Page(web_page["webSocketDebuggerUrl"])
                flash = Page(flash_page["webSocketDebuggerUrl"])
                assert browser.evaluate("document.body.textContent.includes('Chromium ready')")
                browser_image = browser.screenshot()
                browser.evaluate("document.getElementById('next').click()")
                until(lambda: browser.evaluate("document.title === 'Next page'"), "browser link navigation")

                def flash_playing():
                    state = flash.evaluate("""(() => {
                        const player = document.querySelector('ruffle-player');
                        const api = player?.ruffle(1);
                        return {
                            error: document.getElementById('error')?.textContent,
                            ready: api?.readyState,
                            frames: api?.metadata?.numFrames,
                            playing: api && !api.suspended,
                            width: player?.getBoundingClientRect().width
                        };
                    })()""")
                    if state["error"]:
                        raise AssertionError(state["error"])
                    return state if state.get("ready") == 2 and state.get("playing") and state.get("width", 0) > 100 else None

                movie_state = until(flash_playing, "rendered SWF playback")
                assert movie_state.get("frames", 0) > 0, movie_state
                flash_image = flash.screenshot()
                flash.evaluate("togglePlayback()")
                until(lambda: flash.evaluate("document.querySelector('ruffle-player').ruffle(1).suspended"),
                      "Ruffle pause")
                if artifacts:
                    artifacts.mkdir(parents=True, exist_ok=True)
                    (artifacts / "browser.png").write_bytes(browser_image)
                    (artifacts / "flash.png").write_bytes(flash_image)
                print(f"Chromium navigation and Ruffle playback passed ({movie_state['frames']} SWF frames).")
            except BaseException:
                print(log_path.read_text(encoding="utf-8", errors="replace"), file=sys.stderr)
                raise
            finally:
                if browser:
                    browser.close()
                if flash:
                    flash.close()
                process.terminate()
                process.wait(timeout=15)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("hiasm", type=Path)
    parser.add_argument("--movie", type=Path)
    parser.add_argument("--artifacts", type=Path)
    options = parser.parse_args()
    verify(options.hiasm.resolve(), options.movie, options.artifacts)
