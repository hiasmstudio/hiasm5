#ifndef LEGACYBUILD_H_
#define LEGACYBUILD_H_

#include "share.h"
#include "SDK.h"

/*! \file LegacyBuild.h
    \brief build a project of a HiAsm 4 Delphi package

    Reproduces the HiAsm 4 pipeline without touching the installation: the
    package "code" directory is copied to a work directory, the 32-bit bridge
    (src/hiasm4_cgbridge) runs CodeGen.dll and the project maker there,
    GoRC compiles allres.rc, the package compiler builds the project and
    buildCompliteProc moves the result. Windows only.
*/

struct LegacyCompiler {
	ustring path;	/**< compiler executable from hiasm.db (may be relative to the root) */
	ustring cmd;	/**< command template with %fname%, %opath%, %upath%, %curdir%, %oname% */
};

/** Active compiler of a package in Int/hiasm.db of a HiAsm 4 installation */
bool legacyActiveCompiler(const ustring &root, const ustring &pack, LegacyCompiler &compiler);
/** Project maker DLL (make_xxx.dll) of a project entry in Int/hiasm.db */
bool legacyProjectMaker(const ustring &root, const ustring &pack, const ustring &entry, ustring &make);
/** Result extension (exe, dll, ...) of a project entry in Int/hiasm.db; empty when the entry has none */
bool legacyProjectExtension(const ustring &root, const ustring &pack, const ustring &entry, ustring &ext);

struct LegacyBuild {
	ustring root;		/**< HiAsm 4 installation */
	ustring packDir;	/**< package directory, e.g. <root>/Elements/delphi */
	ustring make;		/**< project maker DLL name */
	LegacyCompiler compiler;
	ustring bridge;		/**< hiasm4_cgbridge.exe */
	ustring work;		/**< empty work directory, ANSI-representable path */
	ustring output;		/**< target executable or library */
};

/**
 * Generate, compile and place the project. Progress and errors go to cgt_on_debug.
 * The work directory is left for the caller to remove.
 */
bool buildLegacyProject(MSDK *sdk, const LegacyBuild &build);
/** Removes the work directory of a finished build (copied code, scheme.json, generated units) */
void removeLegacyWorkDirectory(const ustring &work);

#endif /* LEGACYBUILD_H_ */
