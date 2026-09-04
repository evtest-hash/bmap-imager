#pragma once

#include "elevation.h"
#include "sink.h"

#include <string>

namespace bmap {

// Sink that streams write frames to the elevated writer helper. The unprivileged
// process never opens the device itself; it only forwards verified bytes.
class WriterSink : public Sink {
public:
    WriterSink(std::string writerPath, std::string devicePath);
    ~WriterSink() override;

    bool open(std::string* err) override;
    bool writeAt(uint64_t offset, const uint8_t* data, size_t len,
                 std::string* err) override;
    bool close(std::string* err) override;

private:
    std::string writerPath_;
    std::string devicePath_;
    FrameChannel* chan_ = nullptr;
};

}  // namespace bmap
