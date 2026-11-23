#ifndef HIASM_LEGACY_PACK_CATALOG_H
#define HIASM_LEGACY_PACK_CATALOG_H
#include <cstddef>
#include <string>
#include <vector>

struct LegacyPackLocation {
    std::string name;
    std::string directory;
    std::string installation;
};
namespace LegacyPackCatalog {
// Data discovery only: never load a foreign DLL into the GTK host.
std::vector<LegacyPackLocation> discover(const std::string &config, std::vector<std::string> &diagnostics);
std::string decodeText(const char *bytes, size_t size);
// The underscored catalog is authoritative; older installations remain readable.
std::string configurationPath(const std::string &dataDirectory);
bool containsPath(const std::string &directory, const std::string &path);
}
#endif
