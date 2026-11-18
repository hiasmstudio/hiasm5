#ifndef HIASM_RUNNER_PAYLOAD_H
#define HIASM_RUNNER_PAYLOAD_H
#include <cstdint>
#include <cstring>
#include <string>

// Executable | UTF-8 scheme | UTF-8 runtime path | two LE64 lengths | magic.
namespace RunnerPayload {
static const char magic[] = "HIASMR01";
static const std::size_t footerSize = 24;
static const std::uint64_t maxSchemeSize = 64 * 1024 * 1024;
static const std::uint64_t maxRuntimeSize = 1024 * 1024;
inline void appendLength(std::string &data, std::uint64_t value) {
    for(unsigned i = 0; i < 8; ++i)
        data.push_back(static_cast<char>((value >> (i * 8)) & 255));
}
inline std::uint64_t readLength(const char *data) {
    std::uint64_t value = 0;
    for(unsigned i = 0; i < 8; ++i)
        value |= std::uint64_t(static_cast<unsigned char>(data[i])) << (i * 8);
    return value;
}
inline bool append(std::string &binary, const std::string &scheme, const std::string &runtime) {
    if(scheme.empty() || scheme.size() > maxSchemeSize || runtime.size() > maxRuntimeSize) return false;
    binary.append(scheme);
    binary.append(runtime);
    appendLength(binary, scheme.size());
    appendLength(binary, runtime.size());
    binary.append(magic, 8);
    return true;
}
inline bool extract(const std::string &binary, std::string &scheme, std::string &runtime) {
    if(binary.size() < footerSize || std::memcmp(binary.data() + binary.size() - 8, magic, 8)) return false;
    const char *footer = binary.data() + binary.size() - footerSize;
    const std::uint64_t schemeSize = readLength(footer);
    const std::uint64_t runtimeSize = readLength(footer + 8);
    if(!schemeSize || schemeSize > maxSchemeSize || runtimeSize > maxRuntimeSize ||
       schemeSize + runtimeSize > binary.size() - footerSize) return false;
    const std::size_t offset = binary.size() - footerSize - schemeSize - runtimeSize;
    scheme.assign(binary, offset, schemeSize);
    runtime.assign(binary, offset + schemeSize, runtimeSize);
    return true;
}
}
#endif
