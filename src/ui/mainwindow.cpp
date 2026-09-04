#include "mainwindow.h"

#include "device.h"
#include "devicelister.h"
#include "flashworker.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <vector>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("BmapImager"));
    resize(640, 520);

    imageEdit_ = new QLineEdit;
    imageEdit_->setReadOnly(true);
    imageEdit_->setPlaceholderText(QStringLiteral("select a .img / .img.gz / .img.bz2 / .img.xz"));

    bmapEdit_ = new QLineEdit;
    bmapEdit_->setReadOnly(true);
    bmapEdit_->setPlaceholderText(QStringLiteral("select a .bmap file"));

    deviceBox_ = new QComboBox;

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
    auto* layout = new QHBoxLayout(group);
    layout->addWidget(deviceBox_, 1);
    auto* refresh = new QPushButton(QStringLiteral("Refresh"));
    connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshDevices);
    layout->addWidget(refresh);
    return group;
}

void MainWindow::browseImage() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select image"),
        QString(),
        QStringLiteral("Disk images (*.img *.img.gz *.img.bz2 *.img.xz);;All files (*)"));
    if (!path.isEmpty()) {
        imageEdit_->setText(path);
    }
}

void MainWindow::browseBmap() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select bmap"), QString(),
        QStringLiteral("Bmap files (*.bmap);;All files (*)"));
    if (!path.isEmpty()) {
        bmapEdit_->setText(path);
    }
}

void MainWindow::refreshDevices() {
    std::vector<bmap::Device> devices;
    std::string err;
    if (!bmap::listDevices(&devices, &err)) {
        statusLabel_->setText(QString::fromStdString(err));
        return;
    }
    deviceBox_->clear();
    for (const bmap::Device& d : devices) {
        const QString label = QString::fromStdString(d.description + " (" + d.id + ")");
        deviceBox_->addItem(label, QString::fromStdString(d.path));
    }
    statusLabel_->setText(
        devices.empty() ? QStringLiteral("no removable device found")
                        : QStringLiteral("%1 device(s)").arg(devices.size()));
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
        QMessageBox::warning(this, QStringLiteral("BmapImager"),
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
