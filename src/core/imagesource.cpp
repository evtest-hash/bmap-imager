#include "imagesource.h"

#include <archive.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace bmap {

namespace {

class ArchiveImageSource : public ImageSource {
public:
    explicit ArchiveImageSource(struct archive* a) : a_(a) {}

    ~ArchiveImageSource() override {
        if (a_) {
            archive_read_free(a_);
        }
    }

    bool isOpen() const override { return a_ != nullptr; }

    int64_t read(uint8_t* buf, size_t len, std::string* err) override {
        const la_ssize_t n = archive_read_data(a_, buf, len);
        if (n >= 0) {
            pos_ += static_cast<uint64_t>(n);
            return static_cast<int64_t>(n);
        }
        *err = std::string("解压读取失败: ") + archive_error_string(a_);
        return -1;
    }

    bool seek(uint64_t offset, std::string* err) override {
        if (offset < pos_) {
            *err = "不支持向后 seek(仅前向)";
            return false;
        }
        std::vector<uint8_t> discard(64 * 1024);
        while (pos_ < offset) {
            const size_t want = static_cast<size_t>(
                std::min<uint64_t>(discard.size(), offset - pos_));
            const int64_t n = read(discard.data(), want, err);
            if (n < 0) {
                return false;
            }
            if (n == 0) {
                *err = "镜像在预期偏移之前结束";
                return false;
            }
        }
        return true;
    }

private:
    struct archive* a_;
    uint64_t pos_ = 0;
};

}  // namespace

ImageSource* openImageSource(const std::string& path, std::string* err) {
    struct archive* a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);

    if (archive_read_open_filename(a, path.c_str(), 10240) != ARCHIVE_OK) {
        *err = std::string("无法打开镜像: ") + archive_error_string(a);
        archive_read_free(a);
        return nullptr;
    }
    return new ArchiveImageSource(a);
}

}  // namespace bmap
