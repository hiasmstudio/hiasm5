"""Install the Qt 6.9.3 WebEngine extension alongside the CI Qt SDK."""
import hashlib
from pathlib import Path
import shutil
import sys
from tempfile import TemporaryDirectory
from urllib.request import urlopen

import py7zr


ARCHIVES = {
    "linux": (
        "linux_x64/extensions/qtwebengine/693/x86_64/extensions.qtwebengine.693.linux_gcc_64",
        "qtwebengine-Linux-RHEL_8_10-GCC-Linux-RHEL_8_10-X86_64.7z",
        "8ac7e13152a1d3415e8d274a905c692633152286d815b165378d42a829801236",
    ),
    "darwin": (
        "mac_x64/extensions/qtwebengine/693/clang_64/extensions.qtwebengine.693.clang_64",
        "qtwebengine-MacOS-MacOS_15-Clang-MacOS-MacOS_15-X86_64-ARM64.7z",
        "95a1fc974e50936dfe15f74007748c11de030397f419ad5b4f09766374d1648d",
    ),
}


def install_archive(archive, destination, checksum):
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != checksum:
            raise ValueError("Qt WebEngine archive checksum mismatch")
    with py7zr.SevenZipFile(archive) as bundle:
        bundle.extractall(path=destination)


def fetch(destination):
    if not (destination / "lib/cmake/Qt6/Qt6Config.cmake").is_file():
        raise ValueError(f"Qt SDK not found in {destination}")
    repo, name, checksum = ARCHIVES[sys.platform]
    relative = f"online/qtsdkrepository/{repo}/6.9.3-0-202509261208{name}"
    with TemporaryDirectory(prefix="hiasm-qt-webengine-") as directory:
        archive = Path(directory) / name
        for mirror in ("https://ftp.fau.de/qtproject", "https://qt.mirror.constant.com"):
            try:
                with urlopen(f"{mirror}/{relative}", timeout=90) as source, archive.open("wb") as target:
                    shutil.copyfileobj(source, target)
                break
            except OSError as error:
                print(f"{mirror}: {error}", file=sys.stderr)
        else:
            raise RuntimeError("Could not download the Qt WebEngine extension")
        install_archive(archive, destination, checksum)


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.platform not in ARCHIVES:
        raise SystemExit("Usage on Linux or macOS: fetch_qt_webengine.py <Qt 6.9.3 SDK directory>")
    fetch(Path(sys.argv[1]))
