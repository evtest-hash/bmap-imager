#include <QApplication>
#include <QCoreApplication>
#include <QPixmap>
#include <QString>

#include <cstring>

#include "mainwindow.h"
#include "theme.h"

namespace {

// Dev-only: "--screenshot <path>" renders the window to a PNG and exits.
// CI runs it with "-platform offscreen" on all three platforms so UI changes
// are reviewable without a local Qt install.
QString screenshotPath(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--screenshot") == 0) {
            return QString::fromLocal8Bit(argv[i + 1]);
        }
    }
    return QString();
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BmapImager"));
    bmap::theme::apply(&app);

    // QApplication consumes its own switches (-platform, ...) from argv, so
    // parse after constructing it.
    const QString shot = screenshotPath(argc, argv);

    MainWindow w;
    w.show();

    if (!shot.isEmpty()) {
        w.loadSampleState();
        // Let the layout settle before grabbing.
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        return w.grab().save(shot) ? 0 : 1;
    }
    return app.exec();
}
