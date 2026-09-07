#pragma once

#include "device.h"

#include <QMainWindow>

#include <vector>

class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
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

    // Fill the window with placeholder content. Dev-only: used by the
    // --screenshot render so CI produces a populated window, not an empty one.
    void loadSampleState();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    // Re-applies accents when the system flips between light and dark.
    void changeEvent(QEvent* event) override;

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
    QWidget* buildSourceSection();
    QWidget* buildDeviceSection();
    // (Re)applies theme colors to the widgets this window owns.
    void applyAccents();
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
    QLabel* deviceSizeLabel_ = nullptr;
    QLabel* deviceMetaLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPushButton* flashButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;

    // Labels drawn in the secondary text color; re-tinted on a theme change.
    std::vector<QLabel*> mutedLabels_;

    std::vector<bmap::Device> devices_;
    bmap::FlashWorker* worker_ = nullptr;
};
