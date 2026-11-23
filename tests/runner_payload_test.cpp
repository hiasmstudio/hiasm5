#include "RunnerPayload.h"
#include <iostream>
int main() {
    const std::string scheme = u8"Make(_base)\n*Привет, 世界\n";
    const std::string runtime = u8"D:/Тест/HiAsm";
    std::string binary(17, '\0'), decoded, path;
    if(!RunnerPayload::append(binary, scheme, runtime) ||
       !RunnerPayload::extract(binary, decoded, path) || decoded != scheme || path != runtime) return 1;
    std::string truncated = binary.substr(0, binary.size() - 1);
    if(RunnerPayload::extract(truncated, decoded, path)) return 2;
    std::string corrupt = binary;
    corrupt[corrupt.size() - RunnerPayload::footerSize] = char(255);
    if(RunnerPayload::extract(corrupt, decoded, path)) return 3;
    if(RunnerPayload::extract(std::string(20, 'a'), decoded, path)) return 4;
    if(RunnerPayload::append(binary, "", runtime)) return 5;
    std::cout << "OK payload Unicode round trip, truncation, bounds, magic, empty scheme" << std::endl;
    return 0;
}
