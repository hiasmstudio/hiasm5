#include "LegacyPackCatalog.h"
#include <glib.h>
#include <algorithm>
#include <set>
#include <sstream>

namespace {
std::string expandEnvironment(const std::string &line) {
    std::string result;
    for(size_t i=0; i<line.size();) {
        if(line[i] == '%') {
            const auto end = line.find('%', i+1);
            if(end != std::string::npos) {
                const std::string name = line.substr(i+1, end-i-1);
                if(const gchar *value = g_getenv(name.c_str())) { result += value; i=end+1; continue; }
            }
        }
        if(line[i] == '$' && i+1 < line.size() && line[i+1] == '{') {
            const auto end = line.find('}', i+2);
            if(end != std::string::npos) {
                const std::string name = line.substr(i+2, end-i-2);
                if(const gchar *value = g_getenv(name.c_str())) { result += value; i=end+1; continue; }
            }
        }
        if(i == 0 && line[i] == '~' && (line.size()==1 || line[1]=='/' || line[1]=='\\')) { result += g_get_home_dir(); ++i; continue; }
        result += line[i++];
    }
#ifndef G_OS_WIN32
    std::replace(result.begin(), result.end(), '\\', '/');
#endif
    return result;
}
bool identifier(const std::string &name) {
    return !name.empty() && std::all_of(name.begin(),name.end(),[](unsigned char ch) { return g_ascii_isalnum(ch) || ch=='_'; });
}
std::string normalize(const std::string &path, const std::string &base) {
    gchar *value = g_canonicalize_filename(path.c_str(), base.c_str());
    std::string result(value); g_free(value); return result;
}
std::string lower(const std::string &text) {
    gchar *value = g_ascii_strdown(text.c_str(), -1); std::string result(value); g_free(value); return result;
}
}

std::string LegacyPackCatalog::decodeText(const char *bytes, size_t size) {
    if(!bytes) return {};
    if(g_utf8_validate(bytes, size, nullptr)) return std::string(bytes, size);
    gsize written = 0;
    gchar *utf8 = g_convert(bytes, size, "UTF-8", "WINDOWS-1251", nullptr, &written, nullptr);
    if(!utf8) return {};
    std::string result(utf8, written); g_free(utf8); return result;
}

std::string LegacyPackCatalog::configurationPath(const std::string &dataDirectory) {
    const auto current = dataDirectory + "/packs/_hiasm4/paths.ini";
    if(g_file_test(current.c_str(), G_FILE_TEST_IS_REGULAR)) return current;
    return dataDirectory + "/packs/hiasm4/paths.ini";
}

bool LegacyPackCatalog::containsPath(const std::string &directory, const std::string &path) {
    if(directory.empty() || path.empty()) return false;
    gchar *dir = g_canonicalize_filename(directory.c_str(), nullptr);
    gchar *file = g_canonicalize_filename(path.c_str(), nullptr);
    std::string prefix(dir), candidate(file); g_free(dir); g_free(file);
#ifdef G_OS_WIN32
    std::replace(prefix.begin(), prefix.end(), '\\', '/');
    std::replace(candidate.begin(), candidate.end(), '\\', '/');
    prefix = lower(prefix); candidate = lower(candidate);
#endif
    while(prefix.size() > 1 && prefix.back() == '/') prefix.pop_back();
    return candidate == prefix || candidate.compare(0, prefix.size() + 1, prefix + "/") == 0;
}

std::vector<LegacyPackLocation> LegacyPackCatalog::discover(const std::string &config, std::vector<std::string> &diagnostics) {
    std::vector<LegacyPackLocation> result;
    gchar *parent = g_path_get_dirname(config.c_str());
    gchar *absolute = g_canonicalize_filename(parent, nullptr);
    const std::string base(absolute); g_free(parent); g_free(absolute);
    std::vector<std::string> roots;
    if(const gchar *overrideRoots = g_getenv("HIASM4_ROOTS")) {
        std::istringstream input(overrideRoots); std::string line;
        while(std::getline(input,line,';')) roots.push_back(line);
    } else {
        gchar *bytes = nullptr; gsize length = 0; GError *error = nullptr;
        if(!g_file_get_contents(config.c_str(), &bytes, &length, &error)) {
            if(g_file_test(config.c_str(), G_FILE_TEST_EXISTS)) diagnostics.push_back(error ? error->message : "Cannot read paths.ini");
            g_clear_error(&error); return result;
        }
        std::string body = decodeText(bytes,length); g_free(bytes);
        if(body.compare(0,3,"\xef\xbb\xbf")==0) body.erase(0,3);
        std::istringstream input(body); std::string line;
        while(std::getline(input,line)) roots.push_back(line);
    }
    std::set<std::string> seenRoots, seenPacks;
    for(auto line : roots) {
        const auto first = line.find_first_not_of(" \t\r\n");
        if(first == std::string::npos || line[first]=='#' || line[first]==';' || line[first]=='[') continue;
        line = line.substr(first, line.find_last_not_of(" \t\r\n")-first+1);
        if(line.size()>1 && line.front()=='"' && line.back()=='"') line = line.substr(1,line.size()-2);
        const auto root = normalize(expandEnvironment(line),base);
#ifdef G_OS_WIN32
        const auto rootKey = lower(root);
#else
        const auto rootKey = root;
#endif
        if(!seenRoots.insert(rootKey).second) continue;
        std::string elements = root + "/Elements";
        if(!g_file_test(elements.c_str(),G_FILE_TEST_IS_DIR)) elements = root + "/elements";
        GDir *dir = g_dir_open(elements.c_str(),0,nullptr);
        if(!dir) { diagnostics.push_back("HiAsm 4 Elements directory not found: " + root); continue; }
        std::vector<std::string> names;
        while(const gchar *name = g_dir_read_name(dir)) names.push_back(name);
        g_dir_close(dir); std::sort(names.begin(),names.end());
        for(const auto &name : names) {
            if(!identifier(name) || name[0]=='_') continue;
            const auto path = elements + "/" + name;
            if(!g_file_test((path+"/Elements.db").c_str(),G_FILE_TEST_IS_REGULAR) &&
               !g_file_test((path+"/elements.db").c_str(),G_FILE_TEST_IS_REGULAR)) continue;
            if(!g_file_test((path+"/conf").c_str(),G_FILE_TEST_IS_DIR)) continue;
            const auto code = lower(name);
            if(!seenPacks.insert(code).second) { diagnostics.push_back("Duplicate legacy pack ignored: " + code + " in " + root); continue; }
            result.push_back({code,path,root});
        }
    }
    return result;
}
