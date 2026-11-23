# RTCG provenance and build

This directory is the C++ RTCG implementation imported from `UtoECat/hiasm5` commit `4d291dcbafe6e4b516c6d64b54374db8b84fdeac`, now maintained under `src/RTCG`. The upstream implementation exports `buildPrepareProc` and `buildProcessProc` and consumes the HiAsm CodeGenTools table.

The local build uses C++17, position-independent code, and emits `CodeGen.dll` on Windows or `libCodeGen.so` on Unix into the active CMake build's `packs/cpp/compiler`. This keeps the RTCG implementation separate from the web pack's FTCG library. The Qt IDE builds RTCG by default. Use `-DHIASM_BUILD_RTCG_ONLY=ON` to build just the generator.

The ABI record is pointer-sized (`cgt`, `sdk`, `result`). This matches `CGTShare.pas` (`sdk: id_sdk`) and the HiAsm 5 loader. The legacy C header's `DWORD sdk` declaration was corrected because it truncates a 64-bit SDK pointer.

The imported sources are GPL-3.0; see `LICENSE`.
