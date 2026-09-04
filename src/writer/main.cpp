#include "devicelister.h"
#include "rawdevice.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

// Print the single result line to stdout (parsed by the GUI).
void printResult(const std::string& line) {
    std::fputs(line.c_str(), stdout);
    std::fputs("\n", stdout);
    std::fflush(stdout);
}

// Unmount the device volumes (best-effort). Runs as root/admin.
void unmountDevice(const std::string& device) {
#if defined(__APPLE__)
    // diskutil expects /dev/diskN, not /dev/rdiskN.
    std::string disk = device;
    if (disk.rfind("/dev/rdisk", 0) == 0) {
        disk.replace(0, 10, "/dev/disk");
    }
    const std::string cmd = "diskutil unmountDisk " + disk + " >/dev/null 2>&1";
    std::system(cmd.c_str());
#elif defined(__linux__)
    const std::string cmd =
        "umount " + device + " >/dev/null 2>&1; umount " + device +
        "?* >/dev/null 2>&1";
    std::system(cmd.c_str());
#else
    (void)device;  // Windows: volume lock/dismount is added later.
#endif
}

// Read up to n bytes; returns bytes read (0 = EOF, or short read).
size_t readBytes(char* buf, size_t n) {
    size_t got = 0;
    while (got < n) {
#ifdef _WIN32
        const int r = _read(_fileno(stdin), buf + got,
                            static_cast<unsigned>(n - got));
#else
        const ssize_t r = ::read(STDIN_FILENO, buf + got, n - got);
#endif
        if (r <= 0) {
            break;
        }
        got += static_cast<size_t>(r);
    }
    return got;
}

// Read one line (without trailing newline). Returns false on EOF.
bool readLine(std::string* line) {
    line->clear();
    char c;
    while (readBytes(&c, 1) == 1) {
        if (c == '\n') {
            return true;
        }
        if (c != '\r') {
            line->push_back(c);
        }
    }
    return false;
}

// Parse a "W <offset> <length>" header.
bool parseWriteHeader(const std::string& line, uint64_t* offset, uint64_t* len) {
    if (line.rfind("W ", 0) != 0) {
        return false;
    }
    std::istringstream iss(line.substr(2));
    return static_cast<bool>(iss >> *offset >> *len);
}

}  // namespace

int main(int argc, char** argv) {
    std::string device;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    if (argc >= 4 && std::strcmp(argv[1], "--pipe") == 0) {
        // Frames arrive over a named pipe (the GUI relaunches us elevated via
        // runas). Connect and rewire it as stdin/stdout so the rest of this
        // program is identical to the POSIX path.
        HANDLE pipe = CreateFileA(argv[2], GENERIC_READ | GENERIC_WRITE, 0,
                                  nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE) {
            std::fprintf(stderr, "ERR cannot connect to pipe\n");
            return 1;
        }
        const int fd =
            _open_osfhandle(reinterpret_cast<intptr_t>(pipe), _O_RDWR | _O_BINARY);
        _dup2(fd, 0);  // stdin  = pipe (frames)
        _dup2(fd, 1);  // stdout = pipe (result)
        device = argv[3];
    } else if (argc >= 2) {
        device = argv[1];
    } else {
        std::fprintf(stderr, "ERR usage: bmap-writer [--pipe <name>] <device>\n");
        return 1;
    }
#else
    if (argc < 2) {
        std::fprintf(stderr, "ERR usage: bmap-writer <device>\n");
        return 1;
    }
    device = argv[1];
#endif

    // Defense in depth: re-validate the target before touching it.
    std::vector<bmap::Device> devices;
    std::string err;
    if (!bmap::listDevices(&devices, &err)) {
        printResult("ERR cannot enumerate devices: " + err);
        return 1;
    }
    bool found = false;
    for (const bmap::Device& d : devices) {
        if (d.path == device) {
            found = true;
            break;
        }
    }
    if (!found) {
        printResult("ERR device is not a safe target: " + device);
        return 1;
    }

    unmountDevice(device);

    bmap::RawDevice dev;
    if (!dev.open(device, &err)) {
        printResult("ERR " + err);
        return 1;
    }

    // Frame loop: read "W <offset> <len>\n" + payload, write at offset.
    std::string line;
    while (readLine(&line)) {
        if (line.empty()) {
            continue;
        }
        if (line == "END") {
            break;
        }

        uint64_t offset = 0;
        uint64_t len = 0;
        if (!parseWriteHeader(line, &offset, &len)) {
            printResult("ERR unknown frame: " + line);
            return 1;
        }
        if (len > (64ull << 20)) {  // sanity: reject absurd single frames
            printResult("ERR frame too large");
            return 1;
        }
        std::vector<char> buf(static_cast<size_t>(len));
        if (readBytes(buf.data(), static_cast<size_t>(len)) !=
            static_cast<size_t>(len)) {
            printResult("ERR short frame payload");
            return 1;
        }
        if (!dev.writeAt(offset, buf.data(), static_cast<size_t>(len), &err)) {
            printResult("ERR " + err);
            return 1;
        }
    }

    if (!dev.sync(&err)) {
        printResult("ERR " + err);
        return 1;
    }
    dev.close();

    printResult("OK");
    return 0;
}
