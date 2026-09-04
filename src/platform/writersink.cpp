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
    std::string header;
    encodeWriteHeader(offset, len, &header);
    if (!chan_->send(header.data(), header.size(), err)) {
        return false;
    }
    return chan_->send(data, len, err);
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
        msg.erase(msg.find_last_not_of("\r\n") + 1);
        *err = msg.empty() ? "writer failed" : msg;
        return false;
    }
    return true;
}

}  // namespace bmap
