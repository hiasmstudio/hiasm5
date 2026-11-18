![ScreenShot](docs/main_ui.png)

HiAsm - desktop IDE to create programs with a graphical programming

http://hiasm.com

## Building on Windows (MSYS2 UCRT64)

Install the toolchain and dependencies:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-gtk4 mingw-w64-ucrt-x86_64-gtkmm-4.0 \
  mingw-w64-ucrt-x86_64-sqlite3 mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-cmake
```

Build with CMake:

```sh
cmake --preset windows-mingw-gtk4-debug
cmake --build --preset windows-mingw-gtk4-debug
```

Or with Make:

```bat
tools\make_windows.bat
```

The build stages `resources/` and `packs/` into
`build/Windows_MinGW-x64-GTK4-Debug/packs` and regenerates the package element databases.
Runtime compilers belong to `packs/<package>/compiler`; the C++17 RTCG
generator stages `CodeGen.dll`/`libCodeGen.so` there for the `cpp`, `web` and
`modules` packages. The legacy FTCG generator can still be built for
development with `-DHIASM_BUILD_FTCG=ON` when Free Pascal is installed, but it
is not staged into any package.
Run the GTK4 result with `tools\run-debug.bat` (sets the UCRT64 runtime `PATH`).

The helper script wraps configure/build and can also build the installer:

```bat
tools\build_windows.bat debug
tools\build_windows.bat release run
tools\build_windows.bat installer
```

GTK4 is the default Windows backend. Use `tools\build_windows.bat gtk3 debug`
only when the legacy GTK3 profile is required.

### VS Code and Zed

The repository includes project-local configurations for both IDEs:

- VS Code: select a CMake configure/build preset from the CMake Tools status bar.
  The HiAsm: build GTK4 Debug task is the default build task; Run and Debug
  contains HiAsm GTK4 (Debug) and HiAsm GTK3 (Debug) GDB launch profiles.
- Zed: use the task picker (Ctrl+Shift+R) for build/run tasks or the debugger
  panel (Ctrl+Shift+D) for the GTK4/GTK3 GDB profiles. Zed reads
  .zed/tasks.json and .zed/debug.json from the project root.

Both IDE configurations use C:\msys64\ucrt64\bin\gdb.exe and prepend
the MSYS2 UCRT64 runtime directories to PATH. Set MSYS2_ROOT when MSYS2 is
installed elsewhere, or update the IDE environment entries accordingly.
The CMake presets generate compile_commands.json for clangd/IntelliSense in
each selected build directory.

`tools\build_windows_installer.bat gtk4` builds the GTK4 Release tree, copies
the GTK/GLib runtime into `build\windows-installer\gtk4-install` and produces
`build\HiAsm_Setup_GTK4.exe` (requires NSIS 3.x). Use `gtk3` for the legacy
installer. Technical details: [`MARKDOWN/BUILD_WINDOWS.md`](MARKDOWN/BUILD_WINDOWS.md).

On macOS install GTK3/gtkmm3, gtksourceviewmm3 and sqlite with Homebrew, then:

```sh
bash tools/build_macos.sh debug
bash tools/build_macos.sh release run
```

