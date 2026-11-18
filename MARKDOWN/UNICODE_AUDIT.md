# Unicode audit

The project stores UI text, scheme text, package metadata and file names in `Glib::ustring`/UTF-8. The Windows file chooser converts UTF-8 to UTF-16 with `MultiByteToWideChar(CP_UTF8)` and converts the selected path back with `WideCharToMultiByte(CP_UTF8)`. The NSIS installer is explicitly Unicode and writes payload manifests as UTF-8. SQLite package fields are read as UTF-8 strings and the shipped web database contains Cyrillic text, which is covered by the database integrity check.

The source audit found no CP1251 conversion or locale-dependent path conversion in the IDE. FTCG/RTCG receive UTF-8 scheme text through the existing `ustring`/C-string bridge; generated compiler command lines still depend on the selected package compiler accepting UTF-8 paths.

Runtime coverage as of 2026-11-03: scheme_io tests open/save/reload UTF-8 properties and Unicode paths on Windows GTK4/GTK3 and Linux GTK4. RTCG and FTCG compile/run generated output under paths with non-ASCII characters and spaces. C++ string literals cover quotes, backslashes, newlines and control characters. Standalone Windows GTK4 packaging is tested after relocation to paths with Cyrillic text, spaces and an apostrophe on Windows and Linux/Wine; SVG and icon theme loading pass without compiler DLLs.

Third-party Delphi components that still assume ANSI strings remain unverified. External HiAsm 4 metadata is decoded from UTF-8 or CP1251 read-only, and opaque legacy resource values are preserved. No forum Delphi Unicode package was available in the checked sources, so no external package was imported or claimed compatible.
