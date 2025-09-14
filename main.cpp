#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
#include <QWebEngineFullScreenRequest> // <-- include this
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWidget>

class InputEventFilter : public QObject {
  QWebEngineView *view;

public:
  InputEventFilter(QWebEngineView *v) : QObject(v), view(v) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event) override {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      QWidget *widget = qobject_cast<QWidget *>(obj);
      QLineEdit *lineEdit = widget->findChild<QLineEdit *>();

      if (keyEvent->key() == Qt::Key_Escape) {
        widget->deleteLater();
        return true;
      }

      if (keyEvent->key() == Qt::Key_Return ||
          keyEvent->key() == Qt::Key_Enter) {
        if (lineEdit) {
          QString url = lineEdit->text();
          if (!url.startsWith("http://") && !url.startsWith("https://"))
            url = "http://" + url;
          if (view)
            view->setUrl(QUrl(url));
          widget->deleteLater();
        }
        return true;
      }
    }
    return false;
  }
};

void enableDesktopLikeSettings(QWebEnginePage *page) {

  QWebEngineSettings *settings = page->settings();

  settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
  settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);
  settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);
  settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

  settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

  settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
  settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
  settings->setAttribute(QWebEngineSettings::NavigateOnDropEnabled, true);

  settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,
                         false);
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QWebEngineView *view = new QWebEngineView();
  view->setUrl(QUrl("https://www.youtube.com"));
  view->setWindowFlags(Qt::Window);
  enableDesktopLikeSettings(view->page());
  view->page()->runJavaScript("document.documentElement.requestFullscreen()."
                              "catch(e => console.log(e));");
  view->resize(1024, 768);
  view->show();

  QObject::connect(view->page(), &QWebEnginePage::fullScreenRequested,
                   [view](QWebEngineFullScreenRequest request) {
                     if (request.toggleOn())
                       view->window()->showFullScreen();
                     else
                       view->window()->showNormal();

                     request.accept();
                   });

  view->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(
      view, &QWidget::customContextMenuRequested, [view](const QPoint &pos) {
        QMenu menu;

        menu.setStyleSheet(
            "QMenu { background-color: black; color: white; font-size: 18px; }"
            "QMenu::item:selected { background-color: #444444; }");

        QAction *backAction = menu.addAction("Back");
        QAction *forwardAction = menu.addAction("Forward");
        QAction *reloadAction = menu.addAction("Reload");
        menu.addSeparator();
        QAction *customAction = menu.addAction("Custom Action");

        QAction *selected = menu.exec(view->mapToGlobal(pos));
        if (!selected)
          return;

        if (selected == backAction && view->history()->canGoBack())
          view->back();
        else if (selected == forwardAction && view->history()->canGoForward())
          view->forward();
        else if (selected == reloadAction)
          view->reload();
        else if (selected == customAction)
          qDebug() << "Custom action triggered!";
      });

  // Example: Listen for Ctrl+R to reload the page
  QShortcut *reloadShortcut = new QShortcut(QKeySequence("Ctrl+R"), view);
  QObject::connect(reloadShortcut, &QShortcut::activated, [view]() {
    view->reload();
    qDebug() << "Reload triggered!";
  });

  QShortcut *openShortcut = new QShortcut(QKeySequence("Ctrl+O"), view);
  QObject::connect(openShortcut, &QShortcut::activated, [view]() {
    QWidget *inputWidget = new QWidget(view);
    inputWidget->setFixedSize(300, 60);
    inputWidget->setWindowFlags(Qt::Widget);
    inputWidget->setStyleSheet(
        "background-color: black; border: 1px solid white;");

    int x = (view->width() - inputWidget->width()) / 2;
    int y = (view->height() - inputWidget->height()) / 2;
    inputWidget->move(x, y);

    QLineEdit *lineEdit = new QLineEdit(inputWidget);
    lineEdit->setPlaceholderText("Enter URL...");
    lineEdit->setStyleSheet("background-color: white; color: black;");
    lineEdit->setFixedHeight(30);

    QPushButton *goButton = new QPushButton("Go", inputWidget);
    goButton->setFixedHeight(25);
    goButton->setStyleSheet("background-color: gray; color: white;");

    QVBoxLayout *layout = new QVBoxLayout(inputWidget);
    layout->addWidget(lineEdit);
    layout->addWidget(goButton);
    layout->setContentsMargins(5, 5, 5, 5);
    inputWidget->setLayout(layout);

    inputWidget->installEventFilter(new InputEventFilter(view));

    inputWidget->show();
    lineEdit->setFocus();

    QObject::connect(
        goButton, &QPushButton::clicked, [lineEdit, view, inputWidget]() {
          QString url = lineEdit->text();
          if (!url.startsWith("http://") && !url.startsWith("https://"))
            url = "http://" + url;
          view->setUrl(QUrl(url));
          inputWidget->deleteLater();
        });
  });

  return app.exec();
}
