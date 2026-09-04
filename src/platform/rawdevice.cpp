#include "rawdevice.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace bmap {

RawDevice::~RawDevice() { close(); }

#ifdef _WIN32

bool RawDevice::open(const std::string& path, std::string* err) {
    handle_ = CreateFileA(path.c_str(), GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                          OPEN_EXISTING, 0, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
        *err = "cannot open device: " + path;
        handle_ = nullptr;
        return false;
    }
    return true;
}

bool RawDevice::writeAt(uint64_t offset, const void* data, size_t len,
                        std::string* err) {
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(handle_, li, nullptr, FILE_BEGIN)) {
        *err = "seek failed";
        return false;
    }
    size_t written = 0;
    while (written < len) {
        DWORD n = 0;
        if (!WriteFile(handle_, static_cast<const char*>(data) + written,
                       static_cast<DWORD>(len - written), &n, nullptr)) {
            *err = "write failed";
            return false;
        }
        if (n == 0) {
            *err = "write returned 0 bytes";
            return false;
        }
        written += n;
    }
    return true;
}

bool RawDevice::sync(std::string* err) {
    if (!FlushFileBuffers(handle_)) {
        *err = "flush failed";
        return false;
    }
    return true;
}

void RawDevice::close() {
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
}

#else

bool RawDevice::open(const std::string& path, std::string* err) {
    fd_ = ::open(path.c_str(), O_WRONLY);
    if (fd_ < 0) {
        *err = "cannot open device: " + path;
        return false;
    }
#ifdef __APPLE__
    // Bypass the buffer cache for bulk writes to the raw device.
    fcntl(fd_, F_NOCACHE, 1);
#endif
    return true;
}

bool RawDevice::writeAt(uint64_t offset, const void* data, size_t len,
                        std::string* err) {
    size_t written = 0;
    while (written < len) {
        const ssize_t n = ::pwrite(fd_, static_cast<const char*>(data) + written,
                                   len - written,
                                   static_cast<off_t>(offset + written));
        if (n < 0) {
            *err = "write failed";
            return false;
        }
        if (n == 0) {
            *err = "write returned 0 bytes";
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
}

bool RawDevice::sync(std::string* err) {
    if (::fsync(fd_) != 0) {
        *err = "fsync failed";
        return false;
    }
    return true;
}

void RawDevice::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

#endif

}  // namespace bmap
