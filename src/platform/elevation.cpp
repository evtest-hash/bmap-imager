#include "elevation.h"

#include "frame.h"

#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#include <cstdio>
#include <cstring>
#elif defined(Q_OS_LINUX)
#include <QByteArray>
#include <QProcess>
#include <QString>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <cstdio>
#endif

namespace bmap {

#if defined(Q_OS_MACOS)

namespace {

class MacChannel : public FrameChannel {
public:
    MacChannel(FILE* pipe, AuthorizationRef auth) : pipe_(pipe), auth_(auth) {}

    ~MacChannel() override {
        if (pipe_) fclose(pipe_);
        if (auth_) AuthorizationFree(auth_, kAuthorizationFlagDestroyRights);
    }

    bool send(const void* data, size_t len, std::string* err) override {
        if (fwrite(data, 1, len, pipe_) != len) {
            *err = "failed to write to writer";
            return false;
        }
        return true;
    }

    bool finish(std::string* result, std::string* err) override {
        if (fwrite(kFrameEnd, 1, sizeof(kFrameEnd) - 1, pipe_) !=
            sizeof(kFrameEnd) - 1) {
            *err = "failed to finalize writer";
            return false;
        }
        fflush(pipe_);

        std::string out;
        char buf[4096];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), pipe_)) > 0) {
            out.append(buf, n);
        }
        fclose(pipe_);
        pipe_ = nullptr;
        *result = out;
        return true;
    }

private:
    FILE* pipe_;
    AuthorizationRef auth_;
};

}  // namespace

FrameChannel* runWriterElevated(const std::string& writerPath,
                                const std::string& devicePath,
                                std::string* err) {
    AuthorizationItem right = {kAuthorizationRightExecute, 0, nullptr, 0};
    AuthorizationRights rights = {1, &right};
    const AuthorizationFlags flags = kAuthorizationFlagDefaults |
                                     kAuthorizationFlagInteractionAllowed |
                                     kAuthorizationFlagPreAuthorize |
                                     kAuthorizationFlagExtendRights;

    AuthorizationRef auth = nullptr;
    OSStatus st =
        AuthorizationCreate(nullptr, kAuthorizationEmptyEnvironment, flags, &auth);
    if (st == errAuthorizationSuccess) {
        st = AuthorizationCopyRights(auth, &rights, kAuthorizationEmptyEnvironment,
                                     flags, nullptr);
    }
    if (st != errAuthorizationSuccess) {
        *err = (st == errAuthorizationCanceled) ? "authorization cancelled"
                                                : "authorization failed";
        if (auth) AuthorizationFree(auth, kAuthorizationFlagDestroyRights);
        return nullptr;
    }

    char* args[] = {const_cast<char*>(devicePath.c_str()), nullptr};
    FILE* pipe = nullptr;
    st = AuthorizationExecuteWithPrivileges(auth, writerPath.c_str(),
                                            kAuthorizationFlagDefaults, args,
                                            &pipe);
    if (st != errAuthorizationSuccess) {
        *err = (st == errAuthorizationCanceled) ? "authorization cancelled"
                                                : "authorization failed";
        AuthorizationFree(auth, kAuthorizationFlagDestroyRights);
        return nullptr;
    }
    return new MacChannel(pipe, auth);
}

#elif defined(Q_OS_LINUX)

namespace {

class ProcessChannel : public FrameChannel {
public:
    explicit ProcessChannel(QProcess* proc) : proc_(proc) {}

    ~ProcessChannel() override { delete proc_; }

    bool send(const void* data, size_t len, std::string* err) override {
        proc_->write(static_cast<const char*>(data), static_cast<qint64>(len));
        return true;
    }

    bool finish(std::string* result, std::string* err) override {
        proc_->write(kFrameEnd, sizeof(kFrameEnd) - 1);
        proc_->closeWriteChannel();
        if (!proc_->waitForFinished(-1)) {
            *err = "writer did not exit";
            return false;
        }
        *result = proc_->readAllStandardOutput().toStdString();
        return true;
    }

private:
    QProcess* proc_;
};

}  // namespace

FrameChannel* runWriterElevated(const std::string& writerPath,
                                const std::string& devicePath,
                                std::string* err) {
    auto* proc = new QProcess;
    proc->start(QStringLiteral("pkexec"),
                {QString::fromStdString(writerPath),
                 QString::fromStdString(devicePath)});
    if (!proc->waitForStarted(5000)) {
        *err = "failed to launch pkexec (is polkit available?)";
        delete proc;
        return nullptr;
    }
    return new ProcessChannel(proc);
}

#elif defined(Q_OS_WIN)

namespace {

class WinChannel : public FrameChannel {
public:
    WinChannel(HANDLE pipe, HANDLE process) : pipe_(pipe), process_(process) {}

    ~WinChannel() override {
        if (pipe_ != INVALID_HANDLE_VALUE) CloseHandle(pipe_);
        if (process_) CloseHandle(process_);
    }

    bool send(const void* data, size_t len, std::string* err) override {
        size_t off = 0;
        while (off < len) {
            DWORD n = 0;
            if (!WriteFile(pipe_, static_cast<const char*>(data) + off,
                           static_cast<DWORD>(len - off), &n, nullptr)) {
                *err = "pipe write failed";
                return false;
            }
            off += n;
        }
        return true;
    }

    bool finish(std::string* result, std::string* err) override {
        DWORD n = 0;
        if (!WriteFile(pipe_, kFrameEnd, sizeof(kFrameEnd) - 1, &n, nullptr)) {
            *err = "failed to finalize writer";
            return false;
        }
        FlushFileBuffers(pipe_);

        std::string out;
        char buf[4096];
        DWORD r = 0;
        while (ReadFile(pipe_, buf, sizeof(buf), &r, nullptr) && r > 0) {
            out.append(buf, r);
        }
        WaitForSingleObject(process_, INFINITE);
        *result = out;
        return true;
    }

private:
    HANDLE pipe_;
    HANDLE process_;
};

}  // namespace

FrameChannel* runWriterElevated(const std::string& writerPath,
                                const std::string& devicePath,
                                std::string* err) {
    static LONG counter = 0;
    char pipeName[256];
    std::snprintf(pipeName, sizeof(pipeName), "\\\\.\\pipe\\bmapwriter-%lu-%ld",
                  static_cast<unsigned long>(GetCurrentProcessId()),
                  InterlockedIncrement(&counter));

    HANDLE pipe =
        CreateNamedPipeA(pipeName, PIPE_ACCESS_DUPLEX,
                         PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 65536,
                         65536, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
        *err = "CreateNamedPipe failed";
        return nullptr;
    }

    // The helper connects to this pipe for frames (elevated via runas/UAC).
    std::string params = "--pipe \"";
    params += pipeName;
    params += "\" \"";
    params += devicePath;
    params += "\"";

    SHELLEXECUTEINFOA sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = writerPath.c_str();
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        const DWORD e = GetLastError();
        CloseHandle(pipe);
        *err = (e == ERROR_CANCELLED) ? "authorization cancelled"
                                      : "failed to launch elevated writer";
        return nullptr;
    }

    if (!ConnectNamedPipe(pipe, nullptr) &&
        GetLastError() != ERROR_PIPE_CONNECTED) {
        *err = "writer failed to connect";
        CloseHandle(pipe);
        CloseHandle(sei.hProcess);
        return nullptr;
    }
    return new WinChannel(pipe, sei.hProcess);
}

#endif

}  // namespace bmap
