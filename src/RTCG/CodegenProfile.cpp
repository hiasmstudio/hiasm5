#include "direct.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern FILE *openUtf8(const char *path, const char *mode);
extern const char *stringLexem;
extern const char *concatLexem;
extern bool cppStringEscapes;

std::vector<TCGSection> cgSections;
int cgSection = 0;

static std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if(first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static std::string lower(std::string value) {
    for(auto &c : value) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return value;
}

bool cgLoadSections(const char *codePath) {
    cgSections.clear();
    std::string path(codePath ? codePath : "");
    if(!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += "codegen.ini";
    FILE *file = openUtf8(path.c_str(), "rb");
    if(!file) return false;

    TCGSection current;
    bool haveSection = false;
    char line[1024];
    while(fgets(line, sizeof(line), file)) {
        std::string text = trim(line);
        if(text.empty() || text[0] == ';' || text[0] == '#') continue;
        if(text.front() == '[' && text.back() == ']') {
            if(haveSection) cgSections.push_back(current);
            current = TCGSection();
            current.name = trim(text.substr(1, text.size() - 2));
            haveSection = true;
            continue;
        }
        if(!haveSection) continue;
        const auto eq = text.find('=');
        if(eq == std::string::npos) continue;
        const std::string key = trim(text.substr(0, eq));
        const std::string value = trim(text.substr(eq + 1));
        if(key == "entry") current.entry = value;
        else if(key == "name") current.name = value;
        else if(key == "quote") current.quote = value;
        else if(key == "concat") current.concat = value;
        else if(key == "mask") current.mask = value;
        else if(key == "escape") current.escape = value;
    }
    if(haveSection) cgSections.push_back(current);
    fclose(file);
    return !cgSections.empty();
}

int cgFindSection(const char *entry) {
    if(!entry) return 0;
    const std::string wanted = lower(entry);
    for(size_t i = 0; i < cgSections.size(); i++)
        if(lower(cgSections[i].entry) == wanted) {
            // Match the section by name, as HiAsm's readSection does. CSS and
            // HTML share the html writer and therefore the same section bit.
            for(size_t j = 0; j < cgSections.size(); j++)
                if(cgSections[j].name == cgSections[i].name) return static_cast<int>(j);
            return static_cast<int>(i);
        }
    return 0;
}

int cgSectionByName(const char *name) {
    if(!name) return -1;
    const std::string wanted = lower(name);
    for(size_t i = 0; i < cgSections.size(); i++)
        if(lower(cgSections[i].name) == wanted) return static_cast<int>(i);
    return -1;
}

void cgUseSection(int index) {
    if(cgSections.empty()) { cgSection = 0; return; }
    if(index < 0 || static_cast<size_t>(index) >= cgSections.size()) index = 0;
    cgSection = index;
    static std::string quote;
    static std::string concat;
    quote = cgSections[index].quote;
    concat = cgSections[index].concat;
    stringLexem = quote.c_str();
    concatLexem = concat.c_str();
    const std::string escape = lower(cgSections[index].escape);
    cppStringEscapes = escape != "html" && escape != "raw";
}
