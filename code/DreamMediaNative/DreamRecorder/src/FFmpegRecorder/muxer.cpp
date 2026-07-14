//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/muxer.h>
#include <FFmpegRecorder/ffcore.h>
#include <FFmpegRecorder/ffinstance.h>

#include <cstring>
#include <unordered_map>

namespace {
    const char* getFormatName(const char* path) {
        if (path == nullptr) {
            return nullptr;
        }

        static const std::unordered_map<std::string, std::string> formatNameMap = {
            {"rtmp", "flv"}
        };

        const size_t len = std::strlen(path);
        for (const auto& pair : formatNameMap) {
            const std::string& protocol = pair.first;
            size_t protoLen = protocol.size();

            if (len >= protoLen + 3 &&
                std::strncmp(path, protocol.c_str(), protoLen) == 0 &&
                path[protoLen] == ':' &&
                path[protoLen + 1] == '/' &&
                path[protoLen + 2] == '/') {
                return pair.second.c_str();
            }
        }

        return nullptr;
    }
}

void Muxer::init(const char* path) {
    cleanup();

    const char* formatName = getFormatName(path);
    int res = avformat_alloc_output_context2(&fmtCtx_, nullptr, formatName, path);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avformat_alloc_output_context2 failed\n");
        ::exit(1);
    }

    res = avio_open(&fmtCtx_->pb, path, AVIO_FLAG_WRITE);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avio_open failed\n");
        ::exit(1);
    }

    res = avformat_write_header(fmtCtx_, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avformat_write_header failed\n");
        ::exit(1);
    }

    FFINSTANCE()->setFormatContext(fmtCtx_);
}

void Muxer::cleanup() {
    if (fmtCtx_) {
        avformat_free_context(fmtCtx_);
        fmtCtx_ = nullptr;
    }
}

void Muxer::run(std::stop_token st) {
    while (!st.stop_requested()) {

    }

    // 线程结束
    int res = av_write_trailer(fmtCtx_);
    if (res != 0) {
        av_log(nullptr, AV_LOG_ERROR, "av_write_trailer failed\n");
        ::exit(1);
    }

    res = avio_closep(&fmtCtx_->pb);
    if (res != 0) {
        av_log(nullptr, AV_LOG_ERROR, "avio_closep failed\n");
        ::exit(1);
    }
}
