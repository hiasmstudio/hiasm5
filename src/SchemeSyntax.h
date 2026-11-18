#ifndef HIASM_SCHEME_SYNTAX_H
#define HIASM_SCHEME_SYNTAX_H

#include <string>
#include <vector>
#include <cstdlib>
#include <cerrno>
#include <cmath>

// Validate structure before the legacy destructive tokenizer sees it. Property
// values are opaque: their encoded strings can contain braces and parentheses.
inline bool validateSchemeSyntax(const std::string &source, int &errorLine) {
    struct Scope { bool element = false, block = false, needsBlock = false; };
    std::vector<Scope> scopes(1);
    auto number = [](const std::string &s) {
        if(s.empty()) return false;
        char *end = nullptr;
        errno = 0;
        double value = std::strtod(s.c_str(), &end);
        while(*end == ' ' || *end == '\t') ++end;
        return end != s.c_str() && !*end && !errno && std::isfinite(value);
    };
    size_t start = 0;
    errorLine = 0;
    while(start < source.size()) {
        ++errorLine;
        size_t end = source.find_first_of("\r\n", start);
        if(end == std::string::npos) end = source.size();
        std::string s = source.substr(start, end - start);
        start = end;
        if(start < source.size() && source[start] == '\r') ++start;
        if(start < source.size() && source[start] == '\n') ++start;
        const size_t left = s.find_first_not_of(" \t");
        if(left == std::string::npos) continue;
        s = s.substr(left, s.find_last_not_of(" \t") - left + 1);
        if(s[0] == '*') continue;
        auto &scope = scopes.back();
        if(scope.needsBlock && s != "{") return false;
        if(s == "{") {
            if(!scope.needsBlock || scope.block) return false;
            scope.block = true; scope.needsBlock = false;
        } else if(s == "}") {
            if(!scope.block) return false;
            scope.block = false;
        } else if(s == "BEGIN_SDK") {
            if(!scope.element || scope.block) return false;
            scopes.push_back(Scope());
        } else if(s == "END_SDK") {
            if(scopes.size() == 1 || scope.block || scope.needsBlock) return false;
            scopes.pop_back();
        } else if(s.compare(0, 4, "Add(") == 0) {
            if(scope.block || s.back() != ')') return false;
            std::vector<std::string> parts;
            size_t p = 4;
            for(size_t i = p; i < s.size(); ++i) {
                if(s[i] == ',' || i == s.size() - 1) {
                    parts.push_back(s.substr(p, i - p)); p = i + 1;
                }
            }
            if(parts.size() != 4 || parts[0].empty() ||
               !number(parts[1]) || !number(parts[2]) || !number(parts[3])) return false;
            scope.element = true; scope.needsBlock = true;
        } else if(s.compare(0, 4, "Pos(") == 0) {
            // The view position of each scheme, the main one and the BEGIN_SDK ones.
            if(scope.block || s.back() != ')') return false;
        } else if(s.compare(0, 5, "Make(") == 0 || s.compare(0, 4, "ver(") == 0 ||
                  s.compare(0, 8, "MakeExt(") == 0) {
            if(scope.block || s.back() != ')' || scopes.size() != 1) return false;
        } else {
            if(!scope.block) return false;
            if(s.compare(0, 5, "link(") == 0) {
                const auto comma = s.find(',', 5), colon = s.find(':', comma);
                const auto path = s.find(",[", colon);
                if(comma == std::string::npos || colon == std::string::npos ||
                   path == std::string::npos || s.size() < 2 || s.substr(s.size()-2) != "])") return false;
            } else if(s.compare(0, 6, "Point(") == 0 || s.compare(0, 6, "elink(") == 0 ||
                      s.compare(0, 8, "AddHint(") == 0) {
                if(s.back() != ')') return false;
            } else if(s.compare(0, 7, "PColor(") == 0 || s.compare(0, 8, "MakeExt(") == 0) {
                if(s.back() != ')') return false;
            } else if(s.find('=') == std::string::npos) return false;
        }
    }
    return scopes.size() == 1 && !scopes[0].block && !scopes[0].needsBlock;
}
#endif
