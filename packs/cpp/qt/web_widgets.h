#pragma once

#include <QColor>
#include <QByteArray>
#include <QStringList>
#include <QUrl>
#ifdef Q_OS_WIN
#include <QWidget>
#else
#include <QWebEnginePage>
#include <QWebEngineView>
#endif
#include <functional>
#include <memory>

class QPrinter;

#ifdef Q_OS_WIN
class HiasmWebBrowser : public QWidget {
#else
class HiasmWebBrowser : public QWebEngineView {
#endif
public:
    explicit HiasmWebBrowser(QWidget *parent);
#ifdef Q_OS_WIN
    ~HiasmWebBrowser() override;
    QUrl url() const;
    void reload();
    void back();
    void forward();
    void stop();
#endif
    void navigate(const QString &address);
    void navigateInitially(const QString &address);
    void loadText(const QString &html);
    void savePage();
    void printPage();
    void previewPage();
    QString currentHtml() const { return html_; }

    bool silent = false;
    std::function<bool(const QString &)> blockNavigation;
    std::function<bool(const QString &)> blockPopup;
    std::function<void(const QString &)> navigated;
    std::function<void(const QString &)> status;
    std::function<void(const QString &)> title;
    std::function<void(int)> progress;
    std::function<void()> quitRequested;

protected:
    struct Resource {
        QByteArray mime;
        QByteArray bytes;
    };
    using ResourceHandler = std::function<Resource(const QUrl &)>;
    void setResourceOrigin(const QUrl &origin, ResourceHandler handler);
    void setSurfaceColor(const QColor &color);
    void runScript(const QString &script);
    std::function<void(bool)> pageReady;
#ifdef Q_OS_WIN
    void load(const QUrl &address);
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;
#endif

private:
    QString html_;
    quint64 loadSerial_ = 0;
    bool navigationRequested_ = false;
#ifdef Q_OS_WIN
    struct NativeView;
    std::unique_ptr<NativeView> native_;
#else
    std::shared_ptr<QPrinter> printer_;
#endif
};

#ifdef HIASM_QT_RUFFLE
class HiasmFlash : public HiasmWebBrowser {
public:
    static void registerScheme();
    explicit HiasmFlash(QWidget *parent);
    void playMovie(const QString &fileName);
    void togglePlayback();
    void stopMovie();
    void setMovieColor(const QColor &color);
    void goToFrame(int frame);
    int currentFrame() const { return -1; }

private:
    void loadMovie(bool play);
    void evaluate(const QString &script);

    QUrl origin_;
    QString localMovie_;
    QUrl source_;
    QColor color_ = Qt::white;
    QStringList pendingScripts_;
    bool ready_ = false;
};
#endif
