# HiAsm 4: Files And Runtime Research

## Scope

This file records filesystem and binary research for the historical HiAsm installation and the current repository. The reference installation is:

```text
C:\Program Files (x86)\HiAsm
```

The main executable is:

```text
C:\Program Files (x86)\HiAsm\HiAsm.exe
```

Observed file metadata:

- PE32 / i386 application.
- Size: `2291712` bytes.
- Installation timestamp: `24.09.2015 02:20:48`.
- Pascal/Delphi/KOL origin is supported by embedded source paths and RTL/VMT patterns documented in `HiAsm.md`.

## Installation Layout

Relevant top-level components found in the reference installation:

```text
HiAsm.exe
core.dll
elmen.dll
hitools.dll
Elements\
compiler\fpc\
Int\
Plug\
Help\
Settings.cfg
```

The installation also contains runtime libraries and plugins such as:

```text
bass.dll
bszip.dll
jpeg.dll
libmySQL.dll
sqlite3.dll
zlib.dll
lnk_trace.dll
```

The application is therefore a host process with dynamically loaded database, element, compiler, and plugin functionality. The code is not self-contained in `HiAsm.exe`.

## HiAsm.exe Runtime Facts

The existing Ghidra research in `HiAsm.md` establishes these facts:

- The executable is a 32-bit PE x86 binary.
- It uses the KOL/Delphi style runtime.
- It loads configuration from `Settings.cfg`.
- It uses registry data under `Software\\HiAsm`.
- It implements a single-instance workflow using the window/object name `HiAsm`.
- A second instance activates the existing window and sends message `0x403`.
- The loaded `elmen.dll` does not contain the HiAsm window procedure or a `0x403` dispatch path; this protocol belongs to `HiAsm.exe` and cannot be recovered from `elmen.dll` alone.
- The exact `wParam`/`lParam` payload format of message `0x403` remains unresolved and requires the `HiAsm.exe` image, especially its main-window procedure.
- The executable imports dynamic module APIs including `LoadLibraryExA`, `GetProcAddress`, `GetModuleHandleA`, and `FreeLibrary`.
- It uses ANSI file and window APIs extensively, with explicit code-page conversion through `MultiByteToWideChar` and `WideCharToMultiByte`.

Important embedded source paths include:

```text
C:\work\add\kol_hiasm\kol.pas
C:\work\hiasm\SDK.pas
C:\work\hiasm\Element.pas
```

These paths are provenance evidence, not paths required for a current build.

## Database Files And API

The executable references:

```text
.\\int\\hiasm.db
Elements\\<package>\\Elements.db
```

The database API is exposed to `HiAsm.exe` through short thunk functions and global function pointers. Relevant names include:

```text
OpenHiAsmDB
CloseHiAsmDB
OpenDB
CloseDB
EnumPacks
EnumTabs
EnumElements
EnumCompilers
AddCompiler
SetCompiler
GetCompiler
AddPackCompiler
RemovePackCompiler
```

Observed thunk addresses in `HiAsm.exe`:

| Function | Thunk target |
|---|---:|
| `OpenHiAsmDB` | `0x0054BE0C -> [0x005F96F4]` |
| `CloseHiAsmDB` | `0x0054BE14 -> [0x005F96F0]` |
| `EnumPacks` | `0x0054BDB4 -> [0x005F9720]` |
| `OpenDB` | `0x0054BDFC -> [0x005F96FC]` |
| `CloseDB` | `0x0054BE04 -> [0x005F96F8]` |
| `EnumTabs` | `0x0054BD54 -> [0x005F9750]` |
| `EnumElements` | `0x0054BD5C -> [0x005F974C]` |

This separates two database scopes:

1. Global HiAsm database: packs and compilers.
2. Package database: tabs and elements for one selected package.

The implementation of these APIs is not in the short thunks themselves. Further work should inspect the modules that populate the global function pointers, especially `core.dll`, `elmen.dll`, and related runtime DLLs.

Export inspection adds the following evidence:

- `core.dll` exports `newElement` and `init`.
- `elmen.dll` exports the compiler and package operations, including `EnumCompilers`, `AddCompiler`, `SetCompiler`, `GetCompiler`, `EnumPacks`, `AddPack`, `RemovePack`, `AddPackCompiler`, and `RemovePackCompiler`.
- `hitools.dll` exports package/element operations: `InstallPackage`, `InstallElement`, `RemoveElement`, and `LinkFile`.

This makes `elmen.dll` the primary owner candidate for the compiler/package database API, while `core.dll` and `hitools.dll` appear to provide element creation and installation operations. The detailed Ghidra report is maintained separately in [elmenDLL.md](D:/Devs/Cpp/hiasm5/MARKDOWN/HIASM4_RESEARCH/elmenDLL.md).

## CodeGen Loading And ABI

`HiAsm.exe` contains strings and call sites for:

```text
CodeGen.dll
ConfToCode
isReadyForAdd
CheckVersionProc
```

The package-local layout is historically:

```text
Elements\\<package>\\CodeGen.dll
```

The reference installation contains these CodeGen implementations:

```text
Elements\delphi\FTCG_CodeGen.dll
Elements\delphi\FTCG_CodeGen.dpr
Elements\delphi\CodeGen.dll
Elements\delphi\CodeGen.dpr
Elements\web\CodeGen.dll
Elements\web\CodeGen.dpr
Elements\Modules\CodeGen.dll
Elements\RTCG\
Elements\RTCG_Win_fork\
```

Exports verified with LLVM PE inspection:

```text
Elements\web\CodeGen.dll
  hintForElement
  synReadFuncList
  ConfToCode

Elements\delphi\FTCG_CodeGen.dll
  hintForElement
  synReadFuncList
  ConfToCode
```

Both DLLs are Win32/i386 binaries. The current repository loader in `src/ProjectBuilder.cpp` expects a different runtime ABI:

```text
buildPrepareProc
buildProcessProc
```

Therefore the historical `hintForElement/synReadFuncList/ConfToCode` DLL ABI is not directly interchangeable with the current `ProjectBuilder` ABI. This is a central compatibility issue for restoring the old code-generation path.

## FTCG Source Generations

`Elements\delphi\FTCG_CodeGen.dpr` and the imported `src\FTCG\CodeGen.dpr` are not identical source revisions.

Historical Delphi/KOL version:

- Uses `Windows`, `kol`, `Errors`, `MD5`, and old shared types such as `PStrList`.
- References units through paths such as `..\\FTCG\\MD5.pas` and `..\\CGTShare.pas`.
- Size observed: `147704` bytes.

Imported repository version:

- Uses `Classes`, `SysUtils`, and `Math`.
- Defines modernized data structures and uses `TStringList`/`TList`.
- Enables FPC mode switches for newer string and pointer behavior.
- Size observed: `149345` bytes.

The source diff is substantial: approximately 713 changed lines according to `git diff --no-index --stat`.

## Embedded FPC Result

The installation includes an embedded compiler:

```text
compiler\fpc\fpc.exe
compiler\fpc\ppc386.exe
```

Version query result:

```text
Free Pascal Compiler version 1.9.6 [2004/12/31]
Target OS: Win32 for i386
```

A clean test build of the current `src\FTCG\CodeGen.dpr` was attempted with this compiler. It stopped at:

```text
Fatal: Can't find unit Classes
```

The embedded FPC contains old RTL units such as `system.ppu` and `objpas.ppu`, but no compatible `Classes` unit. The current imported FTCG source therefore cannot be built unchanged with the bundled FPC 1.9.6. No `CodeGen.dll` was produced by that test.

A separate build of the historical KOL-based tree was then attempted using the bundled `kol.pas`, `Windows.pas`, `MD5.pas`, `CGTShare.pas`, `Errors.pas`, and `direct.inc`. The compiler successfully compiled `kol.pas`, `MD5.pas`, `CGTShare.pas`, and `Errors.pas`, but the final FTCG compilation failed with 50 source-compatibility errors and produced no DLL. The main failures were:

```text
Can't determine which overloaded function to call
Identifier not found "Format"
Illegal qualifier
```

The failure is therefore no longer a missing-unit problem. FPC 1.9.6 can process the historical KOL dependency tree, but it is not compatible with this particular FTCG source snapshot without source changes or the original Delphi/KOL compiler environment. The temporary build directory was under `%TEMP%` and no repository files were modified.

## RTCG Implementations

Two historical C++ implementations are present:

```text
Elements\RTCG\
Elements\RTCG_Win_fork\
```

The Windows fork includes its own versions of:

```text
Code.cpp
CodeTree.cpp
FuncArgs.cpp
IntFunc.cpp
Objects.cpp
Parser.cpp
Runner.cpp
share.cpp
```

It also contains `ElementData.cpp`, a resource file, and a Windows-specific Makefile. Its Makefile uses:

```text
-DBUILDING_DLL=1
-DCNET
-static -lboost_regex-mgw34-1_55
```

The current `src/` tree does not contain equivalent RTCG modules under these names. The current C++ code generator and the historical RTCG runtime generator are separate implementation areas.

## Web Elements Comparison

The requested historical web element families were compared:

```text
hiDM_*
hiHTM_*
hiTAG_*
```

Results:

```text
Elements\web\code: 62 files
packs\web\code:    62 files
SHA-256 identical:  62 files
Different:           0 files
Missing:             0 files
```

No extraction or copy operation is required for these families. The repository already contains the same web element files as the reference installation.

## CodeGen Export Signatures

The historical Delphi/KOL FTCG source gives the following declarations:

```pascal
function CheckVersionProc(const params: THiAsmVersion): integer;
procedure ConfToCode(const Pack, UName: string);
procedure hintForElement(var p: THintParams); cdecl;
procedure synReadFuncList(var p: TSynParams); cdecl;
```

The `exports` block also exports `buildPrepareProc` and `buildProcessProc`. `CheckVersionProc` accepts a `THiAsmVersion` record by `const` reference and returns `CG_SUCCESS` when `params.build >= 162`, otherwise `CG_INVALID_VERSION`. `ConfToCode` is a Delphi procedure using two managed `string` parameters; it converts a package configuration file into `code\\hi<UName>.hws` and returns no value. It exits without writing when that `.hws` file already exists.

`isReadyForAdd` is absent from `FTCG_CodeGen.dpr`, but is present in the historical general Delphi `Elements\\delphi\\CodeGen.dpr` generation with this declaration:

```pascal
function isReadyForAdd(cgt: PCodeGenTools; const rfd: TRFD_Rec; sdk: id_sdk): boolean; cdecl;
```

It checks the parent element class, inherited class/subtype, generated `.hws`/`.hes` availability, and compiler-specific exclusions. It is exported by that general `CodeGen.dpr`, not by the historical FTCG source. The installed `FTCG_CodeGen.dll` and `web\\CodeGen.dll` export only the legacy visible names observed earlier (`hintForElement`, `synReadFuncList`, `ConfToCode`) when inspected with PE export tooling; the build-process exports require separate verification against the exact DLL revision.

## Additional HiAsm.exe Research Targets

The most useful next investigations for `HiAsm.exe` are:

1. Resolve the target modules behind the DB function-pointer table at `0x005F96F0` through `0x005F9750`.
2. Inspect `core.dll`, `elmen.dll`, and `hitools.dll` exports and imports to identify the owner of the database API.
3. Find the window procedure handling message `0x403` and recover its `wParam`/`lParam` contract.
4. Trace all call sites of `CodeGen.dll`, `ConfToCode`, `isReadyForAdd`, and `CheckVersionProc`.
5. Determine whether the historical `ConfToCode` call is configuration conversion only or part of the build pipeline.
6. Test the old KOL-based FTCG project separately from the modernized `src\FTCG` project.
7. Compare `int\hiasm.db` and package `Elements.db` schemas with the current `packs/elements.db` representation.
8. Record exact SHA-256 hashes of the reference executable and runtime DLLs in a future binary inventory. The reference `HiAsm.exe` size is `2291712` bytes and its installed timestamp is `24.09.2015 02:20:48`; the SHA-256 should be captured in the next binary inventory pass.

## Interpretation

The additional evidence supports this architecture:

```text
HiAsm.exe
  -> runtime DLLs and database API
  -> package databases and element metadata
  -> package-local CodeGen.dll
  -> compiler/runtime-specific generated output
```

The historical installation is therefore a useful compatibility reference, but its DLL ABIs, database layout, compiler generation paths, and 32-bit assumptions must be treated separately from the current GTK/C++ implementation.