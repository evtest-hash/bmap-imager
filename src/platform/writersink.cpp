#include "writersink.h"

#include "frame.h"

#include <utility>

namespace bmap {

WriterSink::WriterSink(std::string writerPath, std::string devicePath)
    : writerPath_(std::move(writerPath)), devicePath_(std::move(devicePath)) {}

WriterSink::~WriterSink() { delete chan_; }

bool WriterSink::open(std::string* err) {
    chan_ = runWriterElevated(writerPath_, devicePath_, err);
    return chan_ != nullptr;
}

bool WriterSink::writeAt(uint64_t offset, const uint8_t* data, size_t len,
                         std::string* err) {
    std::string frame;
    encodeWriteFrame(offset, data, len, &frame);
    return chan_->send(frame.data(), frame.size(), err);
}

bool WriterSink::close(std::string* err) {
    if (!chan_) {
        *err = "writer not open";
        return false;
    }
    std::string result;
    if (!chan_->finish(&result, err)) {
        return false;
    }
    if (result.compare(0, 2, "OK") != 0) {
        std::string msg = result;
        if (msg.compare(0, 4, "ERR ") == 0) {
            msg = msg.substr(4);
        }
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }
        *err = msg.empty() ? "writer failed" : msg;
        return false;
    }
    return true;
}

}  // namespace bmap
