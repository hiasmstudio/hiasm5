#ifndef SDKSERIALIZER_H_
#define SDKSERIALIZER_H_

#include "share.h"
#include "SDK.h"

/**
 * Write a JSON snapshot of the scheme for src/hiasm4_cgbridge, which hosts
 * 32-bit HiAsm 4 CodeGen DLLs. Elements and SDKs are stored in flat tables and
 * referenced by index; the first SDK is the root. Flags use the HiAsm 4
 * encoding. Resource properties carry their original HiAsm 4 text in
 * "resource" ("[ZIP<size><zlib>]" or "[<hex>]").
 * projectPath is the folder where the project maker writes its files (the build work folder), so the
 * scheme folder is never touched; projectName is the scheme name.
 * @return false and a cgt_on_debug message if the file cannot be written
 */
bool sdkSerialize(MSDK *sdk, const ustring &jsonPath, const ustring &projectPath);

#endif /* SDKSERIALIZER_H_ */
