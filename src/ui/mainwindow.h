#pragma once

#include <QMainWindow>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

namespace bmap {
class FlashWorker;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void browseImage();
    void browseBmap();
    void refreshDevices();
    void startFlash();
    void onProgress(int percent);
    void onLog(const QString& line);
    void onWorkerFinished(bool success, const QString& message);

private:
    QWidget* buildSourceGroup();
    QWidget* buildDeviceGroup();
    QString selectedDevicePath() const;
    QString locateWriter() const;
    void setFlashing(bool flashing);

    QLineEdit* imageEdit_ = nullptr;
    QLineEdit* bmapEdit_ = nullptr;
    QComboBox* deviceBox_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QPushButton* flashButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;

    bmap::FlashWorker* worker_ = nullptr;
};
