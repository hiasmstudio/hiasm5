# FTCG provenance

Imported from `UtoECat/hiasm5` commit `4d291dcbafe6e4b516c6d64b54374db8b84fdeac`.

The compiler is optional. Build it with Free Pascal using the `ftcg` CMake target; the main HiAsm build does not require FPC.

Local change: `CodeGen.dpr` checks `FileExists` before loading `hi<Class>.hws`, so a missing
template gives an empty unit instead of an exception (the behaviour of the HiAsm 4 web codegen).
