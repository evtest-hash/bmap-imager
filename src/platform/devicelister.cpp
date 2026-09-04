#include "devicelister.h"

#include <QtGlobal>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QStringList>

namespace bmap {

namespace {

// Run a program synchronously and capture its stdout. Returns false on error.
bool runRaw(const QString& program, const QStringList& args, QByteArray* out,
            std::string* err) {
    QProcess p;
    p.start(program, args);
    if (!p.waitForStarted(5000)) {
        *err = "failed to start " + program.toStdString();
        return false;
    }
    if (!p.waitForFinished(15000)) {
        *err = "timeout running " + program.toStdString();
        return false;
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        *err = program.toStdString() + " exited with error";
        return false;
    }
    *out = p.readAllStandardOutput();
    return true;
}

// Convert a plist-emitting command's output to a JSON document (macOS).
bool runPlistAsJson(const QString& program, const QStringList& args,
                    QJsonDocument* doc, std::string* err) {
    QByteArray plist;
    if (!runRaw(program, args, &plist, err)) {
        return false;
    }
    QProcess p;
    p.start(QStringLiteral("plutil"),
            {QStringLiteral("-convert"), QStringLiteral("json"),
             QStringLiteral("-o"), QStringLiteral("-"), QStringLiteral("-")});
    if (!p.waitForStarted(5000)) {
        *err = "failed to start plutil";
        return false;
    }
    p.write(plist);
    p.closeWriteChannel();
    if (!p.waitForFinished(10000)) {
        *err = "plutil timeout";
        return false;
    }
    const QByteArray json = p.readAllStandardOutput();
    QJsonParseError perr;
    *doc = QJsonDocument::fromJson(json, &perr);
    if (perr.error != QJsonParseError::NoError) {
        *err = "plist JSON parse failed";
        return false;
    }
    return true;
}

}  // namespace

#if defined(Q_OS_MACOS)

namespace {

bool listMacOS(std::vector<Device>* out, std::string* err) {
    QJsonDocument doc;
    if (!runPlistAsJson(QStringLiteral("diskutil"),
                        {QStringLiteral("list"), QStringLiteral("-plist")},
                        &doc, err)) {
        return false;
    }
    const QJsonArray disks =
        doc.object().value(QStringLiteral("AllDisksAndPartitions")).toArray();

    for (const QJsonValue& v : disks) {
        const QJsonObject d = v.toObject();
        const QString id = d.value(QStringLiteral("DeviceIdentifier")).toString();
        const uint64_t size =
            static_cast<uint64_t>(d.value(QStringLiteral("Size")).toDouble());
        if (size == 0 || size > kMaxDeviceBytes) {
            continue;
        }

        QJsonDocument infoDoc;
        if (!runPlistAsJson(
                QStringLiteral("diskutil"),
                {QStringLiteral("info"), QStringLiteral("-plist"), id}, &infoDoc,
                err)) {
            continue;  // best-effort per-device; skip on error
        }
        const QJsonObject info = infoDoc.object();
        const bool internal = info.value(QStringLiteral("Internal")).toBool();
        const bool removableOrExternal =
            info.value(QStringLiteral("RemovableMediaOrExternalDevice")).toBool();
        const QString mediaName = info.value(QStringLiteral("MediaName")).toString();
        const QString virt =
            info.value(QStringLiteral("VirtualOrPhysical")).toString();

        if (internal) continue;
        if (!removableOrExternal) continue;
        if (virt.contains(QStringLiteral("Virtual"), Qt::CaseInsensitive)) continue;
        if (mediaName.toLower().contains(QStringLiteral("apple"))) continue;

        Device dev;
        dev.id = id.toStdString();
        dev.path = ("/dev/r" + id).toStdString();  // raw device for faster writes
        dev.sizeBytes = size;
        dev.description = mediaName.toStdString();
        dev.busType = info.value(QStringLiteral("BusProtocol")).toString().toStdString();
        dev.removable = removableOrExternal;
        out->push_back(std::move(dev));
    }
    return true;
}

}  // namespace

#elif defined(Q_OS_LINUX)

namespace {

bool listLinux(std::vector<Device>* out, std::string* err) {
    QByteArray json;
    if (!runRaw(QStringLiteral("lsblk"),
                {QStringLiteral("-J"), QStringLiteral("-b"),
                 QStringLiteral("-o"), QStringLiteral("NAME,TYPE,RM,TRAN,SIZE,MODEL")},
                &json, err)) {
        return false;
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
    if (perr.error != QJsonParseError::NoError) {
        *err = "lsblk JSON parse failed";
        return false;
    }
    const QJsonArray blocks =
        doc.object().value(QStringLiteral("blockdevices")).toArray();

    for (const QJsonValue& v : blocks) {
        const QJsonObject d = v.toObject();
        if (d.value(QStringLiteral("type")).toString() != QStringLiteral("disk")) {
            continue;
        }
        const QString tran = d.value(QStringLiteral("tran")).toString();
        const bool removable = d.value(QStringLiteral("rm")).toBool();
        const bool isSafeBus = (tran == QStringLiteral("usb")) ||
                               (tran == QStringLiteral("mmc")) ||
                               (tran == QStringLiteral("sd"));
        if (!removable && !isSafeBus) {
            continue;
        }
        const uint64_t size =
            static_cast<uint64_t>(d.value(QStringLiteral("size")).toDouble());
        if (size == 0 || size > kMaxDeviceBytes) {
            continue;
        }

        const QString name = d.value(QStringLiteral("name")).toString();
        QString model = d.value(QStringLiteral("model")).toString();
        if (model.isEmpty()) {
            model = name;
        }

        Device dev;
        dev.id = name.toStdString();
        dev.path = ("/dev/" + name).toStdString();
        dev.sizeBytes = size;
        dev.description = model.toStdString();
        dev.busType = tran.toStdString();
        dev.removable = removable || isSafeBus;
        out->push_back(std::move(dev));
    }
    return true;
}

}  // namespace

#elif defined(Q_OS_WIN)

namespace {

bool listWindows(std::vector<Device>* out, std::string* err) {
    QByteArray json;
    if (!runRaw(QStringLiteral("powershell.exe"),
                {QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                 QStringLiteral("Get-Disk | Select-Object "
                                "Number,FriendlyName,BusType,Size,IsSystem,IsBoot "
                                "| ConvertTo-Json")},
                &json, err)) {
        return false;
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
    if (perr.error != QJsonParseError::NoError) {
        *err = "Get-Disk JSON parse failed";
        return false;
    }
    QJsonArray arr;
    if (doc.isArray()) {
        arr = doc.array();
    } else if (doc.isObject()) {
        arr = QJsonArray{doc.object()};
    } else {
        return true;  // no disks
    }

    for (const QJsonValue& v : arr) {
        const QJsonObject d = v.toObject();
        if (d.value(QStringLiteral("IsSystem")).toBool() ||
            d.value(QStringLiteral("IsBoot")).toBool()) {
            continue;
        }
        const QString bus = d.value(QStringLiteral("BusType")).toString();
        if (bus != QStringLiteral("USB") && bus != QStringLiteral("SD")) {
            continue;
        }
        const uint64_t size =
            static_cast<uint64_t>(d.value(QStringLiteral("Size")).toDouble());
        if (size == 0 || size > kMaxDeviceBytes) {
            continue;
        }

        const int number = d.value(QStringLiteral("Number")).toInt();
        Device dev;
        dev.id = std::to_string(number);
        dev.path = "\\\\.\\PhysicalDrive" + std::to_string(number);
        dev.sizeBytes = size;
        dev.description =
            d.value(QStringLiteral("FriendlyName")).toString().toStdString();
        dev.busType = bus.toStdString();
        dev.removable = true;
        out->push_back(std::move(dev));
    }
    return true;
}

}  // namespace

#endif

bool listDevices(std::vector<Device>* out, std::string* err) {
#if defined(Q_OS_MACOS)
    return listMacOS(out, err);
#elif defined(Q_OS_LINUX)
    return listLinux(out, err);
#elif defined(Q_OS_WIN)
    return listWindows(out, err);
#else
    *err = "unsupported platform";
    return false;
#endif
}

}  // namespace bmap
