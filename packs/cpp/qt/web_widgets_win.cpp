#include "web_widgets.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLibrary>
#include <QPalette>
#include <QPointer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <atomic>
#include <utility>

#include <windows.h>
#include <rpcndr.h>
#include <shlwapi.h>
#include <wrl/client.h>

#if defined(__MINGW32__) && defined(__clang__)
// Preserve the interface UUID attributes stripped by the MinGW MIDL macro.
#  pragma push_macro("MIDL_INTERFACE")
#  undef MIDL_INTERFACE
#  define MIDL_INTERFACE(id) struct __declspec(uuid(id))
#endif
#include <WebView2.h>
#if defined(__MINGW32__) && defined(__clang__)
#  pragma pop_macro("MIDL_INTERFACE")
#  undef __uuidof
#endif

namespace {
using Microsoft::WRL::ComPtr;

template<class Interface, class Signature = decltype(&Interface::Invoke)> class Handler;

template<class Interface, class... Args>
class Handler<Interface, HRESULT (STDMETHODCALLTYPE Interface::*)(Args...)> final : public Interface {
public:
    explicit Handler(std::function<HRESULT(Args...)> callback) : callback_(std::move(callback)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **value) override {
        if (!value) return E_POINTER;
        *value = nullptr;
        if (id != IID_IUnknown && id != __uuidof(Interface)) return E_NOINTERFACE;
        *value = static_cast<Interface *>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const auto remaining = --refs_;
        if (!remaining) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE Invoke(Args... args) override { return callback_(args...); }

private:
    std::atomic<ULONG> refs_{1};
    std::function<HRESULT(Args...)> callback_;
};

template<class Interface, class F> ComPtr<Interface> handler(F &&function) {
    ComPtr<Interface> result;
    result.Attach(new Handler<Interface>(std::forward<F>(function)));
    return result;
}

template<class View, class EventHandler, class F>
void watch(View *view, HRESULT (STDMETHODCALLTYPE View::*add)(EventHandler *, EventRegistrationToken *),
           F &&function) {
    EventRegistrationToken token{};
    (view->*add)(handler<EventHandler>(std::forward<F>(function)).Get(), &token);
}

template<class Interface, class Source> ComPtr<Interface> query(Source *object) {
    ComPtr<Interface> result;
    object->QueryInterface(__uuidof(Interface), reinterpret_cast<void **>(result.GetAddressOf()));
    return result;
}

template<class Object, class Getter> QString wideProperty(Object *object, Getter getter) {
    LPWSTR value = nullptr;
    if (FAILED((object->*getter)(&value))) return {};
    const auto text = QString::fromWCharArray(value);
    CoTaskMemFree(value);
    return text;
}
}

struct HiasmWebBrowser::NativeView {
    explicit NativeView(HiasmWebBrowser *widget) : owner(widget) {}
    ~NativeView() {
        if (controller) controller->Close();
        page.Reset();
        controller.Reset();
        environment.Reset();
        if (comInitialized) CoUninitialize();
    }

    HiasmWebBrowser *owner;
    ComPtr<ICoreWebView2Environment> environment;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> page;
    QList<std::function<void()>> pending;
    QColor background;
    QUrl resourceOrigin;
    ResourceHandler resourceHandler;
    bool comInitialized = false;
    bool failed = false;
    bool hostNavigation = false;

    void reportFailure(const QString &reason) {
        if (failed) return;
        failed = true;
        pending.clear();
        auto *layout = new QVBoxLayout(owner);
        auto *label = new QLabel(reason, owner);
        label->setWordWrap(true);
        layout->addWidget(label);
        label->show();
        qWarning().noquote() << reason;
        if (owner->status) owner->status(reason);
    }

    void initialize() {
        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
            reportFailure(owner->tr("Could not initialize COM for WebView2."));
            return;
        }
        comInitialized = true;
        static QLibrary loader(QDir(QCoreApplication::applicationDirPath()).filePath("WebView2Loader.dll"));
        const auto create = reinterpret_cast<decltype(&CreateCoreWebView2EnvironmentWithOptions)>(
            loader.resolve("CreateCoreWebView2EnvironmentWithOptions"));
        if (!create) {
            reportFailure(owner->tr("WebView2Loader.dll is missing from the application directory."));
            return;
        }
        const auto profile = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                 .filePath("webview2").toStdWString();
        QPointer<HiasmWebBrowser> alive(owner);
        const auto ready = handler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [alive](HRESULT code, ICoreWebView2Environment *value) -> HRESULT {
                if (!alive) return S_OK;
                if (FAILED(code) || !value) {
                    alive->native_->reportFailure(alive->tr("Could not start Microsoft Edge WebView2 Runtime."));
                    return S_OK;
                }
                alive->native_->environment = value;
                alive->native_->createController();
                return S_OK;
            });
        if (FAILED(create(nullptr, profile.c_str(), nullptr, ready.Get())))
            reportFailure(owner->tr("Could not start Microsoft Edge WebView2 Runtime."));
    }

    void createController() {
        QPointer<HiasmWebBrowser> alive(owner);
        const auto ready = handler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [alive](HRESULT code, ICoreWebView2Controller *value) -> HRESULT {
                if (!alive) {
                    if (value) value->Close();
                    return S_OK;
                }
                auto &view = *alive->native_;
                if (FAILED(code) || !value || FAILED(value->get_CoreWebView2(view.page.GetAddressOf()))) {
                    view.reportFailure(alive->tr("Could not create the WebView2 browser control."));
                    return S_OK;
                }
                view.controller = value;
                view.connectPage();
                view.resize();
                view.applyColor();
                for (const auto &action : std::exchange(view.pending, {})) action();
                return S_OK;
            });
        if (FAILED(environment->CreateCoreWebView2Controller(reinterpret_cast<HWND>(owner->winId()), ready.Get())))
            reportFailure(owner->tr("Could not create the WebView2 browser control."));
    }

    void resize() {
        if (!controller) return;
        const auto window = reinterpret_cast<HWND>(owner->winId());
        RECT bounds{};
        GetClientRect(window, &bounds);
        EnableWindow(window, owner->isEnabled());
        controller->put_Bounds(bounds);
        controller->put_IsVisible(owner->isVisible());
        controller->NotifyParentWindowPositionChanged();
    }

    void applyColor() {
        if (!controller || !background.isValid()) return;
        const auto color = query<ICoreWebView2Controller2>(controller.Get());
        if (color) color->put_DefaultBackgroundColor({255, BYTE(background.red()),
                                                        BYTE(background.green()), BYTE(background.blue())});
    }

    void enqueue(std::function<void()> action) {
        if (page) action();
        else if (!failed) pending.append(std::move(action));
    }

    template<class Action> void navigate(Action action) {
        enqueue([this, action] {
            hostNavigation = true;
            if (FAILED(action(page.Get()))) hostNavigation = false;
        });
    }

    void evaluate(const QString &script, std::function<void(const QString &)> reply = {}) {
        const auto callback = handler<ICoreWebView2ExecuteScriptCompletedHandler>(
            [reply = std::move(reply)](HRESULT code, LPCWSTR result) -> HRESULT {
                if (reply && SUCCEEDED(code) && result) reply(QString::fromWCharArray(result));
                return S_OK;
            });
        page->ExecuteScript(script.toStdWString().c_str(), callback.Get());
    }

    void connectPage() {
        QPointer<HiasmWebBrowser> alive(owner);
        watch(page.Get(), &ICoreWebView2::add_NavigationStarting, [alive](auto *, auto *args) -> HRESULT {
            if (!alive) return S_OK;
            BOOL fromUser = FALSE;
            BOOL redirected = FALSE;
            args->get_IsUserInitiated(&fromUser);
            args->get_IsRedirected(&redirected);
            const bool initiatedHere = std::exchange(alive->native_->hostNavigation, false);
            if (!initiatedHere && fromUser && !redirected && alive->blockNavigation &&
                alive->blockNavigation(wideProperty(args, &ICoreWebView2NavigationStartingEventArgs::get_Uri))) {
                args->put_Cancel(TRUE);
                return S_OK;
            }
            ++alive->loadSerial_;
            alive->html_.clear();
            if (alive->progress) alive->progress(0);
            return S_OK;
        });
        watch(page.Get(), &ICoreWebView2::add_ContentLoading, [alive](auto *, auto *) -> HRESULT {
            if (alive && alive->progress) alive->progress(70);
            return S_OK;
        });
        watch(page.Get(), &ICoreWebView2::add_NavigationCompleted, [alive](auto *, auto *args) -> HRESULT {
            if (!alive) return S_OK;
            BOOL success = FALSE;
            args->get_IsSuccess(&success);
            if (alive->progress) alive->progress(100);
            if (alive->pageReady) alive->pageReady(success);
            if (!success) {
                if (alive->status) alive->status(alive->tr("Could not load %1").arg(alive->url().toDisplayString()));
                return S_OK;
            }
            const auto address = alive->url().toString();
            alive->native_->evaluate("document.documentElement?.outerHTML || ''",
                [alive, serial = alive->loadSerial_, address](const QString &result) {
                    if (!alive || serial != alive->loadSerial_) return;
                    const auto values = QJsonDocument::fromJson("[" + result.toUtf8() + "]").array();
                    alive->html_ = values.isEmpty() ? QString() : values.at(0).toString();
                    if (alive->navigated) alive->navigated(address);
                });
            return S_OK;
        });
        watch(page.Get(), &ICoreWebView2::add_DocumentTitleChanged, [alive](auto *sender, auto *) -> HRESULT {
            if (alive && alive->title) alive->title(wideProperty(sender, &ICoreWebView2::get_DocumentTitle));
            return S_OK;
        });
        watch(page.Get(), &ICoreWebView2::add_WindowCloseRequested, [alive](auto *, auto *) -> HRESULT {
            if (alive && alive->quitRequested) alive->quitRequested();
            return S_OK;
        });
        watch(page.Get(), &ICoreWebView2::add_NewWindowRequested, [alive](auto *, auto *args) -> HRESULT {
            args->put_Handled(TRUE);
            if (!alive) return S_OK;
            const auto address = wideProperty(args, &ICoreWebView2NewWindowRequestedEventArgs::get_Uri);
            if (alive->blockPopup && alive->blockPopup(address)) return S_OK;
            auto *popup = new HiasmWebBrowser(alive);
            popup->setWindowFlag(Qt::Window);
            popup->setAttribute(Qt::WA_DeleteOnClose);
            popup->resize(800, 600);
            popup->title = [popup](const QString &name) { popup->setWindowTitle(name); };
            popup->quitRequested = [popup] { popup->close(); };
            popup->navigate(address);
            popup->show();
            return S_OK;
        });
        const auto statusPage = query<ICoreWebView2_12>(page.Get());
        if (statusPage) {
            watch(statusPage.Get(), &ICoreWebView2_12::add_StatusBarTextChanged,
                [alive](auto *sender, auto *) -> HRESULT {
                    if (alive && alive->status) {
                        const auto view = query<ICoreWebView2_12>(sender);
                        alive->status(wideProperty(view.Get(), &ICoreWebView2_12::get_StatusBarText));
                    }
                    return S_OK;
                });
        }
        if (!resourceHandler) return;
        const auto pattern = (resourceOrigin.toString() + "/*").toStdWString();
        const auto resourcePage = query<ICoreWebView2_22>(page.Get());
        if (resourcePage)
            resourcePage->AddWebResourceRequestedFilterWithRequestSourceKinds(pattern.c_str(),
                COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL, COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
        else
            page->AddWebResourceRequestedFilter(pattern.c_str(), COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
        watch(page.Get(), &ICoreWebView2::add_WebResourceRequested, [alive](auto *, auto *args) -> HRESULT {
            if (alive) alive->native_->serve(args);
            return S_OK;
        });
    }

    void serve(ICoreWebView2WebResourceRequestedEventArgs *args) {
        ComPtr<ICoreWebView2WebResourceRequest> request;
        if (FAILED(args->get_Request(request.GetAddressOf()))) return;
        const QUrl address(wideProperty(request.Get(), &ICoreWebView2WebResourceRequest::get_Uri));
        if (address.scheme() != resourceOrigin.scheme() || address.host() != resourceOrigin.host()) return;
        const auto resource = resourceHandler(address);
        const bool found = !resource.mime.isEmpty();
        ComPtr<IStream> stream;
        if (found) stream.Attach(SHCreateMemStream(
            reinterpret_cast<const BYTE *>(resource.bytes.constData()), UINT(resource.bytes.size())));
        const auto headers = QString::fromLatin1("Content-Type: " + resource.mime).toStdWString();
        ComPtr<ICoreWebView2WebResourceResponse> response;
        if (SUCCEEDED(environment->CreateWebResourceResponse(stream.Get(), found ? 200 : 404,
                      found ? L"OK" : L"Not Found", headers.c_str(), response.GetAddressOf())))
            args->put_Response(response.Get());
    }

    void print(COREWEBVIEW2_PRINT_DIALOG_KIND kind) {
        enqueue([this, kind] {
            const auto printing = query<ICoreWebView2_16>(page.Get());
            if (printing) printing->ShowPrintUI(kind);
        });
    }
};

HiasmWebBrowser::HiasmWebBrowser(QWidget *parent) : QWidget(parent), native_(std::make_unique<NativeView>(this)) {
    setAttribute(Qt::WA_NativeWindow);
    QTimer::singleShot(0, this, [this] { native_->initialize(); });
}

HiasmWebBrowser::~HiasmWebBrowser() = default;

void HiasmWebBrowser::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    native_->resize();
}

void HiasmWebBrowser::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    native_->resize();
}

void HiasmWebBrowser::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    native_->resize();
}

void HiasmWebBrowser::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) native_->resize();
}

QUrl HiasmWebBrowser::url() const {
    return native_->page ? QUrl(wideProperty(native_->page.Get(), &ICoreWebView2::get_Source)) : QUrl();
}

void HiasmWebBrowser::load(const QUrl &address) {
    native_->navigate([address](ICoreWebView2 *page) {
        return page->Navigate(QString::fromLatin1(address.toEncoded()).toStdWString().c_str());
    });
}

void HiasmWebBrowser::navigate(const QString &address) {
    if (address.trimmed().isEmpty()) return;
    navigationRequested_ = true;
    const QFileInfo local(address);
    load(local.isFile() ? QUrl::fromLocalFile(local.absoluteFilePath()) : QUrl::fromUserInput(address));
}

void HiasmWebBrowser::navigateInitially(const QString &address) {
    if (!navigationRequested_) navigate(address);
}

void HiasmWebBrowser::loadText(const QString &html) {
    navigationRequested_ = true;
    native_->navigate([html](ICoreWebView2 *page) { return page->NavigateToString(html.toStdWString().c_str()); });
}

void HiasmWebBrowser::reload() {
    native_->navigate([](ICoreWebView2 *page) { return page->Reload(); });
}

void HiasmWebBrowser::back() {
    native_->navigate([](ICoreWebView2 *page) {
        BOOL allowed = FALSE;
        page->get_CanGoBack(&allowed);
        return allowed ? page->GoBack() : E_FAIL;
    });
}

void HiasmWebBrowser::forward() {
    native_->navigate([](ICoreWebView2 *page) {
        BOOL allowed = FALSE;
        page->get_CanGoForward(&allowed);
        return allowed ? page->GoForward() : E_FAIL;
    });
}

void HiasmWebBrowser::stop() {
    native_->enqueue([this] { native_->page->Stop(); });
}

void HiasmWebBrowser::savePage() {
    const auto path = QFileDialog::getSaveFileName(this, tr("Save web page"), {}, tr("HTML (*.html)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    const auto bytes = html_.toUtf8();
    if (!(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit()) && status)
        status(tr("Could not save %1").arg(path));
}

void HiasmWebBrowser::printPage() {
    native_->print(COREWEBVIEW2_PRINT_DIALOG_KIND_SYSTEM);
}

void HiasmWebBrowser::previewPage() {
    native_->print(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER);
}

void HiasmWebBrowser::setResourceOrigin(const QUrl &origin, ResourceHandler handler) {
    native_->resourceOrigin = origin;
    native_->resourceHandler = std::move(handler);
}

void HiasmWebBrowser::setSurfaceColor(const QColor &color) {
    native_->background = color;
    native_->applyColor();
}

void HiasmWebBrowser::runScript(const QString &script) {
    native_->enqueue([this, script] { native_->evaluate(script); });
}
