#include "web_widgets.h"

#include <QDir>
#include <QBuffer>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>
#include <QPrinter>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

namespace {
class BrowserPage final : public QWebEnginePage {
public:
    explicit BrowserPage(HiasmWebBrowser *view) : QWebEnginePage(view), view_(view) {}

private:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool mainFrame) override {
        if (mainFrame && type == NavigationTypeLinkClicked && view_->blockNavigation)
            return !view_->blockNavigation(url.toString());
        return true;
    }

    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message,
                                  int line, const QString &source) override {
        if (!view_->silent) QWebEnginePage::javaScriptConsoleMessage(level, message, line, source);
    }

    HiasmWebBrowser *view_;
};

bool saveBytes(const QString &fileName, const QByteArray &bytes) {
    QSaveFile file(fileName);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}

QUrl userUrl(const QString &address) {
    const QFileInfo local(address);
    if (local.isFile()) return QUrl::fromLocalFile(local.absoluteFilePath());
    return QUrl::fromUserInput(address);
}

class FlashResources final : public QWebEngineUrlSchemeHandler {
public:
    explicit FlashResources(QWebEngineProfile *profile) : QWebEngineUrlSchemeHandler(profile) {
        profile->installUrlSchemeHandler("hiasm-flash", this);
    }
    QHash<QString, std::function<void(QWebEngineUrlRequestJob *)>> routes;

private:
    void requestStarted(QWebEngineUrlRequestJob *request) override {
        const auto address = request->requestUrl();
        const auto origin = request->initiator();
        const auto route = routes.constFind(address.host());
        if (route == routes.cend() ||
            (!origin.isEmpty() && origin != QUrl("null") &&
             (origin.scheme() != address.scheme() || origin.host() != address.host()))) {
            request->fail(QWebEngineUrlRequestJob::RequestDenied);
            return;
        }
        (*route)(request);
    }
};
}

HiasmWebBrowser::HiasmWebBrowser(QWidget *parent) : QWebEngineView(parent) {
    setPage(new BrowserPage(this));
    connect(this, &QWebEngineView::loadStarted, this, [this] {
        ++loadSerial_;
        html_.clear();
    });
    connect(this, &QWebEngineView::loadFinished, this, [this](bool success) {
        if (pageReady) pageReady(success);
        if (!success) {
            if (status) status(tr("Could not load %1").arg(url().toDisplayString()));
            return;
        }
        const auto address = url();
        page()->toHtml([view = QPointer<HiasmWebBrowser>(this), serial = loadSerial_, address](const QString &html) {
            if (!view || serial != view->loadSerial_) return;
            view->html_ = html;
            if (view->navigated) view->navigated(address.toString());
        });
    });
    connect(this, &QWebEngineView::titleChanged, this, [this](const QString &name) {
        if (title) title(name);
    });
    connect(this, &QWebEngineView::loadProgress, this, [this](int percent) {
        if (progress) progress(percent);
    });
    connect(page(), &QWebEnginePage::windowCloseRequested, this, [this] {
        if (quitRequested) quitRequested();
    });
    connect(page(), &QWebEnginePage::linkHovered, this, [this](const QString &link) {
        if (status) status(link);
    });
    connect(page(), &QWebEnginePage::newWindowRequested, this, [this](QWebEngineNewWindowRequest &request) {
        if (blockPopup && blockPopup(request.requestedUrl().toString())) return;
        auto *popup = new HiasmWebBrowser(this);
        popup->silent = silent;
        popup->setWindowFlag(Qt::Window);
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->resize(800, 600);
        connect(popup->page(), &QWebEnginePage::windowCloseRequested, popup, &QWidget::close);
        connect(popup, &QWebEngineView::titleChanged, popup, &QWidget::setWindowTitle);
        request.openIn(popup->page());
        popup->show();
    });
}

void HiasmWebBrowser::navigate(const QString &address) {
    if (address.trimmed().isEmpty()) return;
    navigationRequested_ = true;
    load(userUrl(address));
}

void HiasmWebBrowser::navigateInitially(const QString &address) {
    if (!navigationRequested_) navigate(address);
}

void HiasmWebBrowser::loadText(const QString &html) {
    navigationRequested_ = true;
    setHtml(html, url().isLocalFile() || url().scheme() == "http" || url().scheme() == "https"
        ? url() : QUrl());
}

void HiasmWebBrowser::setResourceOrigin(const QUrl &origin, ResourceHandler handler) {
    static auto *resources = new FlashResources(QWebEngineProfile::defaultProfile());
    const auto host = origin.host();
    resources->routes.insert(host, [handler = std::move(handler)](QWebEngineUrlRequestJob *request) {
        auto resource = handler(request->requestUrl());
        if (resource.mime.isEmpty()) {
            request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }
        auto *buffer = new QBuffer(request);
        buffer->setData(std::move(resource.bytes));
        buffer->open(QIODevice::ReadOnly);
        request->reply(resource.mime, buffer);
    });
    connect(this, &QObject::destroyed, resources, [host] { resources->routes.remove(host); });
}

void HiasmWebBrowser::setSurfaceColor(const QColor &color) {
    page()->setBackgroundColor(color);
}

void HiasmWebBrowser::runScript(const QString &script) {
    page()->runJavaScript(script);
}

void HiasmWebBrowser::savePage() {
    const auto path = QFileDialog::getSaveFileName(this, tr("Save web page"), {}, tr("HTML (*.html)"));
    if (path.isEmpty()) return;
    page()->toHtml([view = QPointer<HiasmWebBrowser>(this), path](const QString &html) {
        if (!view) return;
        if (!saveBytes(path, html.toUtf8()) && view->status)
            view->status(tr("Could not save %1").arg(path));
    });
}

void HiasmWebBrowser::printPage() {
    if (printer_) return;
    printer_ = std::make_shared<QPrinter>(QPrinter::HighResolution);
    connect(this, &QWebEngineView::printFinished, this, [this](bool success) {
        printer_.reset();
        if (!success && status) status(tr("Could not print the web page"));
    }, Qt::SingleShotConnection);
    print(printer_.get());
}

void HiasmWebBrowser::previewPage() {
    page()->printToPdf([view = QPointer<HiasmWebBrowser>(this)](const QByteArray &pdf) {
        if (!view) return;
        const auto directory = std::make_shared<QTemporaryDir>();
        const auto path = directory->filePath("preview.pdf");
        if (pdf.isEmpty() || !directory->isValid() || !saveBytes(path, pdf)) {
            if (view->status) view->status(tr("Could not create a print preview"));
            return;
        }
        auto *preview = new QWebEngineView(view);
        preview->setWindowFlag(Qt::Window);
        preview->setAttribute(Qt::WA_DeleteOnClose);
        preview->setWindowTitle(tr("Print preview"));
        preview->resize(800, 600);
        connect(preview, &QObject::destroyed, [directory] {});
        preview->load(QUrl::fromLocalFile(path));
        preview->show();
    });
}
