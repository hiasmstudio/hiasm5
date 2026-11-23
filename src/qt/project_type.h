#pragma once
#include <QString>

enum class ProjectType { Unknown, Console, Form, Worker, Library, WindowsLibrary, Plugin, WindowsService, ControlPanel, ComExtension, Html, Css, JavaScript, Php, HisScript, ModuleSource };

inline ProjectType projectType(const QString &entry, const QString &pack) {
    if (pack == "web") {
        if (entry == "HtmlEntryPoint") return ProjectType::Html;
        if (entry == "CSSEntryPoint") return ProjectType::Css;
        if (entry == "JavaEntryPoint") return ProjectType::JavaScript;
        if (entry == "EntryPoint") return ProjectType::Php;
        return ProjectType::Unknown;
    }
    if (pack == "modules") {
        if (entry == "HISEntryPoint") return ProjectType::HisScript;
        if (entry == "MakeEntryPoint") return ProjectType::ModuleSource;
        return ProjectType::Unknown;
    }
    if (pack != "cpp") return ProjectType::Unknown;
    if (entry == "Entry" || entry == "Console") return ProjectType::Console;
    if (entry == "MainForm") return ProjectType::Form;
    if (entry == "Daemon" || entry == "Service") return ProjectType::Worker;
    if (entry == "Library") return ProjectType::Library;
    if (entry == "WinDll") return ProjectType::WindowsLibrary;
    if (entry == "Plugin") return ProjectType::Plugin;
    if (entry == "WinService") return ProjectType::WindowsService;
    if (entry == "CplApp") return ProjectType::ControlPanel;
    if (entry == "ComExt") return ProjectType::ComExtension;
    return ProjectType::Unknown;
}

inline bool sharedArtifact(ProjectType t) {
    return t == ProjectType::Library || t == ProjectType::WindowsLibrary || t == ProjectType::Plugin ||
           t == ProjectType::ControlPanel || t == ProjectType::ComExtension;
}

inline bool textArtifact(ProjectType t) {
    return t == ProjectType::Html || t == ProjectType::Css || t == ProjectType::JavaScript ||
           t == ProjectType::Php || t == ProjectType::HisScript || t == ProjectType::ModuleSource;
}

inline QString artifactSuffix(ProjectType t) {
    switch (t) {
    case ProjectType::Html: return ".html";
    case ProjectType::Css: return ".css";
    case ProjectType::JavaScript: return ".js";
    case ProjectType::Php: return ".php";
    case ProjectType::HisScript: return ".his";
    case ProjectType::ModuleSource: return ".cpp";
    default: break;
    }
    if (t == ProjectType::ControlPanel) return ".cpl";
#ifdef Q_OS_WIN
    return sharedArtifact(t) ? ".dll" : ".exe";
#elif defined(Q_OS_MACOS)
    return sharedArtifact(t) ? ".dylib" : "";
#else
    return sharedArtifact(t) ? ".so" : "";
#endif
}
