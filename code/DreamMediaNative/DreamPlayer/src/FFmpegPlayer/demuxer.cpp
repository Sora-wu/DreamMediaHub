//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <mutex>
#include <FFmpegPlayer/demuxer.h>
#include <FFmpegPlayer/ffcore.h>

using namespace::std::literals;

bool Demuxer::open(const char* url) {
    int res = avformat_open_input(&fmtCtx_, url, nullptr, nullptr);
    if (res < 0) {
        std::string errorStr = FFUtils::getError(res);
        av_log(nullptr, AV_LOG_FATAL, "Failed to open input, error code: %d, error message: %s", res, errorStr.c_str());
        return false;
    }

    res = avformat_find_stream_info(fmtCtx_, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_WARNING, "%s: could not find codec parameters\n", url);
        return false;
    }

    for (int i = 0; i < fmtCtx_->nb_streams; ++i) {
        streams_.push_back(fmtCtx_->streams[i]);
    }

    duration_ = fmtCtx_->duration / AV_TIME_BASE;
    isEof_ = false;

    return true;
}

void Demuxer::run() {
    AVPacket pkt{};
    int res = 0;

    while (!isExit_) {
        res = av_read_frame(fmtCtx_, &pkt);
        if (res < 0) {
            if ((res == AVERROR_EOF || avio_feof(fmtCtx_->pb)) && !isEof_) {
                // TODO: 放入空包
                isEof_ = true;
            }
            if (fmtCtx_->pb && fmtCtx_->pb->error) {
                isExit_ = true;
                break;
            }

            std::unique_lock lock(mtxRead_);
            cvRead_.wait_for(lock, 10ms);
            continue;
        }

        handle(&pkt);
        av_packet_unref(&pkt);
    }
}
