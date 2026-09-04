#include "mainwindow.h"

#include "devicelister.h"
#include "flashworker.h"

#include <QApplication>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include <string>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("BmapImager"));
    resize(640, 520);

    imageEdit_ = new QLineEdit;
    imageEdit_->setReadOnly(true);
    imageEdit_->setPlaceholderText(
        QStringLiteral("select or drop a .img / .img.gz / .img.bz2 / .img.xz"));

    bmapEdit_ = new QLineEdit;
    bmapEdit_->setReadOnly(true);
    bmapEdit_->setPlaceholderText(QStringLiteral("select or drop a .bmap file"));

    deviceBox_ = new QComboBox;
    deviceDetailLabel_ = new QLabel;
    deviceDetailLabel_->setStyleSheet(QStringLiteral("color: gray;"));

    statusLabel_ = new QLabel(QStringLiteral("ready"));
    progressBar_ = new QProgressBar;
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    logView_ = new QPlainTextEdit;
    logView_->setReadOnly(true);

    flashButton_ = new QPushButton(QStringLiteral("Flash"));
    connect(flashButton_, &QPushButton::clicked, this, &MainWindow::startFlash);

    cancelButton_ = new QPushButton(QStringLiteral("Cancel"));
    cancelButton_->setEnabled(false);
    connect(cancelButton_, &QPushButton::clicked, this, [this]() {
        if (worker_) {
            worker_->cancel();
            cancelButton_->setEnabled(false);
            statusLabel_->setText(QStringLiteral("cancelling..."));
        }
    });

    connect(deviceBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::updateDeviceDetail);

    // Route drops to the window rather than the text widgets.
    imageEdit_->setAcceptDrops(false);
    bmapEdit_->setAcceptDrops(false);
    logView_->setAcceptDrops(false);
    setAcceptDrops(true);

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->addWidget(buildSourceGroup());
    layout->addWidget(buildDeviceGroup());

    auto* progressRow = new QHBoxLayout;
    progressRow->addWidget(progressBar_, 1);
    progressRow->addWidget(statusLabel_);
    layout->addLayout(progressRow);

    layout->addWidget(logView_, 1);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(cancelButton_);
    buttonRow->addWidget(flashButton_);
    layout->addLayout(buttonRow);

    setCentralWidget(central);
    refreshDevices();
}

QWidget* MainWindow::buildSourceGroup() {
    auto* group = new QGroupBox(QStringLiteral("Source"));
    auto* layout = new QVBoxLayout(group);

    auto* imageRow = new QHBoxLayout;
    imageRow->addWidget(new QLabel(QStringLiteral("Image")));
    imageRow->addWidget(imageEdit_, 1);
    auto* imageBtn = new QPushButton(QStringLiteral("Browse..."));
    connect(imageBtn, &QPushButton::clicked, this, &MainWindow::browseImage);
    imageRow->addWidget(imageBtn);
    layout->addLayout(imageRow);

    auto* bmapRow = new QHBoxLayout;
    bmapRow->addWidget(new QLabel(QStringLiteral("Bmap")));
    bmapRow->addWidget(bmapEdit_, 1);
    auto* bmapBtn = new QPushButton(QStringLiteral("Browse..."));
    connect(bmapBtn, &QPushButton::clicked, this, &MainWindow::browseBmap);
    bmapRow->addWidget(bmapBtn);
    layout->addLayout(bmapRow);

    return group;
}

QWidget* MainWindow::buildDeviceGroup() {
    auto* group = new QGroupBox(QStringLiteral("Target device"));
    auto* layout = new QVBoxLayout(group);
    auto* row = new QHBoxLayout;
    row->addWidget(deviceBox_, 1);
    auto* refresh = new QPushButton(QStringLiteral("Refresh"));
    connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    row->addWidget(refresh);
    layout->addLayout(row);
    layout->addWidget(deviceDetailLabel_);
    return group;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            const QString p = url.toLocalFile().toLower();
            if (p.endsWith(QStringLiteral(".bmap")) ||
                p.endsWith(QStringLiteral(".img")) ||
                p.endsWith(QStringLiteral(".gz")) ||
                p.endsWith(QStringLiteral(".bz2")) ||
                p.endsWith(QStringLiteral(".xz"))) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString p = url.toLocalFile();
        if (p.endsWith(QStringLiteral(".bmap"))) {
            setBmap(p);
        } else {
            setImage(p);
        }
    }
    event->acceptProposedAction();
}

void MainWindow::browseImage() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select image"), QString(),
        QStringLiteral("Disk images (*.img *.img.gz *.img.bz2 *.img.xz);;All files (*)"));
    if (!path.isEmpty()) {
        setImage(path);
    }
}

void MainWindow::browseBmap() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select bmap"), QString(),
        QStringLiteral("Bmap files (*.bmap);;All files (*)"));
    if (!path.isEmpty()) {
        setBmap(path);
    }
}

void MainWindow::setImage(const QString& path) {
    imageEdit_->setText(path);
    const QString bmap = findBmapForImage(path);
    if (!bmap.isEmpty()) {
        bmapEdit_->setText(bmap);
    }
}

void MainWindow::setBmap(const QString& path) {
    bmapEdit_->setText(path);
    const QString image = findImageForBmap(path);
    if (!image.isEmpty()) {
        imageEdit_->setText(image);
    }
}

QString MainWindow::findBmapForImage(const QString& image) const {
    QString stem = image;
    const QStringList compressions = {QStringLiteral(".gz"), QStringLiteral(".bz2"),
                                      QStringLiteral(".xz"), QStringLiteral(".lzma"),
                                      QStringLiteral(".zst")};
    for (const QString& c : compressions) {
        if (stem.endsWith(c)) {
            stem = stem.left(stem.size() - c.size());
            break;
        }
    }
    const QString candidate = stem + QStringLiteral(".bmap");
    return QFileInfo(candidate).exists() ? candidate : QString();
}

QString MainWindow::findImageForBmap(const QString& bmap) const {
    QString stem = bmap;
    if (stem.endsWith(QStringLiteral(".bmap"))) {
        stem = stem.left(stem.size() - 5);
    }
    const QStringList candidates = {
        stem, stem + QStringLiteral(".gz"), stem + QStringLiteral(".bz2"),
        stem + QStringLiteral(".xz"), stem + QStringLiteral(".lzma"),
        stem + QStringLiteral(".zst")};
    for (const QString& c : candidates) {
        if (QFileInfo(c).exists()) {
            return c;
        }
    }
    return QString();
}

void MainWindow::refreshDevices() {
    std::string err;
    if (!bmap::listDevices(&devices_, &err)) {
        statusLabel_->setText(QString::fromStdString(err));
        return;
    }
    deviceBox_->clear();
    for (const bmap::Device& d : devices_) {
        const QString label =
            QString::fromStdString(d.description + " (" + d.id + ")");
        deviceBox_->addItem(label, QString::fromStdString(d.path));
    }
    statusLabel_->setText(devices_.empty()
                              ? QStringLiteral("no removable device found")
                              : QStringLiteral("%1 device(s)").arg(devices_.size()));
    updateDeviceDetail(deviceBox_->currentIndex());
}

void MainWindow::updateDeviceDetail(int index) {
    if (index < 0 || index >= static_cast<int>(devices_.size())) {
        deviceDetailLabel_->clear();
        return;
    }
    const bmap::Device& d = devices_[static_cast<size_t>(index)];
    QStringList parts;
    parts << formatSize(d.sizeBytes);
    if (!d.busType.empty()) {
        parts << QString::fromStdString(d.busType);
    }
    if (d.removable) {
        parts << QStringLiteral("removable");
    }
    deviceDetailLabel_->setText(parts.join(QStringLiteral(" · ")));
}

QString MainWindow::formatSize(uint64_t bytes) {
    const double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gb >= 1.0) {
        return QString::number(gb, 'f', 1) + QStringLiteral(" GB");
    }
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    return QString::number(mb, 'f', 0) + QStringLiteral(" MB");
}

QString MainWindow::selectedDevicePath() const {
    return deviceBox_->currentData().toString();
}

QString MainWindow::locateWriter() const {
    const QString dir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("bmap-writer.exe");
#else
    const QString name = QStringLiteral("bmap-writer");
#endif
    const QString candidate = dir + QLatin1Char('/') + name;
    if (QFileInfo(candidate).isExecutable()) {
        return candidate;
    }
    return name;  // fall back to PATH
}

void MainWindow::startFlash() {
    if (worker_) {
        return;  // already flashing
    }
    const QString image = imageEdit_->text();
    const QString bmap = bmapEdit_->text();
    const QString device = selectedDevicePath();
    if (image.isEmpty() || bmap.isEmpty() || device.isEmpty()) {
        QMessageBox::warning(
            this, QStringLiteral("BmapImager"),
            QStringLiteral("Select an image, a bmap file, and a target device."));
        return;
    }

    const QMessageBox::StandardButton r = QMessageBox::warning(
        this, QStringLiteral("Confirm write"),
        QStringLiteral("This will irreversibly erase all data on:\n\n%1\n\nContinue?")
            .arg(device),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r != QMessageBox::Yes) {
        return;
    }

    const QString writer = locateWriter();
    logView_->clear();
    setFlashing(true);

    worker_ = new bmap::FlashWorker(image, bmap, device, writer);
    connect(worker_, &bmap::FlashWorker::progress, this, &MainWindow::onProgress);
    connect(worker_, &bmap::FlashWorker::logLine, this, &MainWindow::onLog);
    connect(worker_, &bmap::FlashWorker::flashFinished, this,
            &MainWindow::onWorkerFinished);
    connect(worker_, &QThread::finished, worker_, &QObject::deleteLater);
    worker_->start();
}

void MainWindow::onProgress(int percent) {
    progressBar_->setValue(percent);
}

void MainWindow::onLog(const QString& line) {
    logView_->appendPlainText(line);
}

void MainWindow::onWorkerFinished(bool success, const QString& message) {
    setFlashing(false);
    worker_ = nullptr;
    if (success) {
        progressBar_->setValue(100);
        statusLabel_->setText(QStringLiteral("done"));
        QMessageBox::information(this, QStringLiteral("BmapImager"), message);
    } else {
        statusLabel_->setText(QStringLiteral("failed"));
        QMessageBox::critical(this, QStringLiteral("BmapImager"), message);
    }
}

void MainWindow::setFlashing(bool flashing) {
    flashButton_->setEnabled(!flashing);
    cancelButton_->setEnabled(flashing);
    imageEdit_->setEnabled(!flashing);
    bmapEdit_->setEnabled(!flashing);
    deviceBox_->setEnabled(!flashing);
    if (flashing) {
        statusLabel_->setText(QStringLiteral("flashing..."));
    }
}
