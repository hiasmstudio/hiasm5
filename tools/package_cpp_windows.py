"""Package a Windows C++17 executable with its GTK4 DLL/data closure."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
from collections import deque
from pathlib import Path


# Imports provided by Windows. Unknown non-API-set imports fail closed rather
# than silently creating a distribution with a missing third-party DLL.
SYSTEM_DLLS = set("""
advapi32 avrt bcrypt bcryptprimitives cabinet cfgmgr32 comctl32 comdlg32
crypt32 cryptbase cryptnet d2d1 d3d11 d3d12 d3dcompiler_47 dbghelp dnsapi
dcomp dsound dwmapi dwrite dxgi gdi32 gdiplus hid imm32 iphlpapi kernel32
kernelbase mf mfplat mfreadwrite mfuuid mpr mscms msvcrt msimg32 ncrypt
netapi32 normaliz ntdll ole32 oleacc oleaut32 opengl32 powrprof propsys
psapi rpcrt4 secur32 setupapi shell32 shlwapi shcore user32 userenv
uiautomationcore usp10 ucrtbase uxtheme version win32u windowscodecs winhttp wininet winmm
winspool wintrust wldap32 ws2_32 wtsapi32 vulkan-1
""".split()) - {"vulkan-1"}  # Vulkan is copied from the runtime, not assumed.


def system_dll(name):
    lower = name.casefold()
    return (lower == "winspool.drv" or lower.startswith(("api-ms-win-", "ext-ms-"))
            or lower.removesuffix(".dll") in SYSTEM_DLLS)


def imports(path, objdump):
    result = subprocess.run([objdump, "-p", str(path)], check=True, capture_output=True,
                            text=True, encoding="utf-8", errors="replace", env={**os.environ, "LC_ALL": "C"})
    return re.findall(r"DLL Name:\s*(\S+)", result.stdout)


def package(executable, runtime, output, objdump, wine, prefix):
    executable = executable.resolve(strict=True)
    runtime = runtime.resolve(strict=True)
    output = output.resolve()
    if output.exists():
        raise ValueError("Output directory must not exist; choose a new directory")
    if output.is_relative_to(runtime):
        raise ValueError("Output must be outside the input runtime directory")
    with executable.open("rb") as source:
        if source.read(2) != b"MZ":
            raise ValueError("Input is not a Windows executable")
        source.seek(0x3c)
        offset = int.from_bytes(source.read(4), "little")
        source.seek(offset)
        if source.read(6) != b"PE\0\0\x64\x86":
            raise ValueError("Input must be a Windows x86-64 PE executable")
    output.mkdir(parents=True)
    dlls = {}
    for directory in (runtime, runtime / "bin"):
        for path in directory.glob("*.dll"):
            dlls.setdefault(path.name.casefold(), path)
    queue = deque()
    copied = set()

    def copy_binary(source, relative):
        destination = output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        queue.append(destination)

    copy_binary(executable, executable.name)
    loaders = sorted((runtime / "lib/gdk-pixbuf-2.0/2.10.0/loaders").glob("*.dll"))
    if not any(p.name.casefold() in {"libpixbufloader-svg.dll", "pixbufloader_svg.dll"} for p in loaders):
        raise ValueError("Runtime lacks the SVG loader required by GTK icon themes; use a complete GTK4 runtime")
    for loader in loaders:
        copy_binary(loader, loader.relative_to(runtime))
    query = next((p for p in (runtime / "gdk-pixbuf-query-loaders.exe", runtime / "bin/gdk-pixbuf-query-loaders.exe") if p.is_file()), None)
    if not query:
        raise ValueError("Runtime must contain gdk-pixbuf-query-loaders.exe to generate a relocatable cache")
    query_name = "hiasm-query-loaders.exe"
    copy_binary(query, query_name)
    while queue:
        binary = queue.popleft()
        for name in imports(binary, objdump):
            if system_dll(name) or name.casefold() in copied:
                continue
            source = dlls.get(name.casefold())
            if not source:
                raise ValueError(f"Missing runtime DLL {name}, imported by {binary.name}")
            copied.add(name.casefold())
            copy_binary(source, source.name)
    for relative in ("share/glib-2.0/schemas", "share/icons", "share/gtk-4.0", "share/fontconfig", "etc/fonts", "share/locale"):
        source = runtime / relative
        if source.is_dir():
            shutil.copytree(source, output / relative)
    for source, relative in ((runtime / "licenses", "licenses/runtime"), (runtime / "share/licenses", "licenses/toolchain")):
        if source.is_dir():
            shutil.copytree(source, output / relative)
    manifest = runtime / "manifest.json"
    if manifest.is_file():
        shutil.copy2(manifest, output / "licenses/toolchain-manifest.json")
    environment = dict(os.environ)
    environment["WINEPATH"] = ""
    for key in ("GDK_PIXBUF_MODULE_FILE", "GDK_PIXBUF_MODULEDIR", "GTK_DATA_PREFIX", "GTK_EXE_PREFIX", "GTK_PATH", "GIO_EXTRA_MODULES"):
        environment.pop(key, None)
    if prefix:
        environment["WINEPREFIX"] = str(prefix.resolve())
    command = [str(output / query_name)] if os.name == "nt" else [wine, str(output / query_name)]
    command += [p.relative_to(runtime).as_posix() for p in loaders]
    result = subprocess.run(command, cwd=output, env=environment, capture_output=True,
                            text=True, encoding="utf-8", errors="replace", timeout=90)
    if result.returncode:
        raise ValueError(f"Loader query failed ({result.returncode}): {result.stderr.strip()}")
    if "g_module_open() failed" in result.stderr:
        raise ValueError("Cannot load packaged image loaders: " + result.stderr)
    if '"svg"' not in result.stdout:
        raise ValueError("Packaged image loader cache contains no SVG loader")
    # The query receives only relative module paths, so the resulting cache is
    # relocatable. Its informational LoaderDir comment is not used at runtime.
    cache = output / "lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
    cache.write_text(result.stdout, encoding="utf-8")
    (output / query_name).unlink()
    files = []
    for path in sorted(output.rglob("*")):
        if path.is_file():
            files.append({"path": path.relative_to(output).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    (output / "distribution.json").write_text(json.dumps({"format": 1, "entry": executable.name,
        "architecture": "windows-x86_64", "dlls": len(copied), "files": files}, indent=2) + "\n", encoding="utf-8")
    print(f"Packaged {executable.name}: {len(copied)} runtime DLLs, {len(files)} files -> {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--objdump", default="objdump")
    parser.add_argument("--wine", default="wine")
    parser.add_argument("--wine-prefix", type=Path)
    args = parser.parse_args()
    try:
        package(args.executable, args.runtime, args.output, args.objdump, args.wine, args.wine_prefix)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Packaging failed: {error}\n")


if __name__ == "__main__":
    main()
