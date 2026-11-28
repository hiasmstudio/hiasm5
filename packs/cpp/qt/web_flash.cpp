#include "web_widgets.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#ifndef Q_OS_WIN
#include <QWebEngineUrlScheme>
#endif
#include <utility>

namespace {
QString argumentList(const QJsonArray &arguments) {
    const auto json = QJsonDocument(arguments).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}
}

void HiasmFlash::registerScheme() {
#ifndef Q_OS_WIN
    QWebEngineUrlScheme scheme("hiasm-flash");
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::CorsEnabled |
                    QWebEngineUrlScheme::FetchApiAllowed);
    QWebEngineUrlScheme::registerScheme(scheme);
#endif
}

HiasmFlash::HiasmFlash(QWidget *parent) : HiasmWebBrowser(parent) {
    const auto host = QUuid::createUuid().toString(QUuid::WithoutBraces);
#ifdef Q_OS_WIN
    origin_.setScheme("https");
    origin_.setHost(host + ".hiasm-flash.test");
#else
    origin_.setScheme("hiasm-flash");
    origin_.setHost(host);
#endif
    blockNavigation = [](const QString &) { return true; };
    blockPopup = blockNavigation;
    setResourceOrigin(origin_, [this](const QUrl &url) -> Resource {
        const auto path = url.path();
        QString file;
        QByteArray mime;
        if (!localMovie_.isEmpty() && path == source_.path()) {
            file = localMovie_;
            mime = "application/x-shockwave-flash";
        } else if (path.count('/') == 1) {
            file = ":/hiasm/ruffle" + path;
            if (path == "/ruffle_player.html") mime = "text/html";
            else if (path.endsWith(".js")) mime = "text/javascript";
            else if (path.endsWith(".wasm")) mime = "application/wasm";
        }
        QFile input(file);
        if (mime.isEmpty() || !input.open(QIODevice::ReadOnly)) return {};
        return {mime, input.readAll()};
    });
    pageReady = [this](bool success) {
        if (!success) {
            qWarning("Flash: could not load the local Ruffle player.");
            return;
        }
        ready_ = true;
        for (const auto &script : std::exchange(pendingScripts_, {})) runScript(script);
    };
    auto wrapper = origin_;
    wrapper.setPath("/ruffle_player.html");
    load(wrapper);
}

void HiasmFlash::evaluate(const QString &script) {
    if (ready_) runScript(script);
    else pendingScripts_.append(script);
}

void HiasmFlash::loadMovie(bool play) {
    evaluate("loadMovie(" + argumentList({source_.toString(QUrl::FullyEncoded), color_.name(), play}) + ")");
}

void HiasmFlash::playMovie(const QString &fileName) {
    if (fileName.trimmed().isEmpty()) return;
    const QUrl address(fileName);
    if (address.scheme() == "http" || address.scheme() == "https") {
        localMovie_.clear();
        source_ = address;
    } else {
        const QFileInfo file(address.isLocalFile() ? address.toLocalFile() : fileName);
        if (!file.isFile()) {
            qWarning().noquote() << "Flash: SWF file not found:" << file.absoluteFilePath();
            return;
        }
        localMovie_ = file.absoluteFilePath();
        source_ = origin_;
        source_.setPath("/movie/" + file.fileName());
    }
    loadMovie(true);
}

void HiasmFlash::togglePlayback() {
    if (!source_.isEmpty()) evaluate("togglePlayback()");
}

void HiasmFlash::stopMovie() {
    if (!source_.isEmpty()) loadMovie(false);
}

void HiasmFlash::setMovieColor(const QColor &color) {
    color_ = color;
    setSurfaceColor(color_);
    evaluate("setMovieColor(" + argumentList({color_.name()}) + ")");
}

void HiasmFlash::goToFrame(int) {
    qWarning("Flash: the public Ruffle Web API does not expose timeline seeking.");
}
