#pragma once

#include "device.h"

#include <QMainWindow>

#include <vector>

class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;

namespace bmap {
class FlashWorker;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void browseImage();
    void browseBmap();
    void refreshDevices();
    void updateDeviceDetail(int index);
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
    void setImage(const QString& path);
    void setBmap(const QString& path);

    // Auto-pairing: look for a same-named bmap/image next to the chosen file.
    QString findBmapForImage(const QString& image) const;
    QString findImageForBmap(const QString& bmap) const;

    static QString formatSize(uint64_t bytes);

    QLineEdit* imageEdit_ = nullptr;
    QLineEdit* bmapEdit_ = nullptr;
    QComboBox* deviceBox_ = nullptr;
    QLabel* deviceDetailLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* flashButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;

    std::vector<bmap::Device> devices_;
    bmap::FlashWorker* worker_ = nullptr;
};
