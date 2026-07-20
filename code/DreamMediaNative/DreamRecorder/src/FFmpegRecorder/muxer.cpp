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
    constexpr uint32_t VIDEO_STREAM_INDEX = 0;
    constexpr uint32_t AUDIO_STREAM_INDEX = 1;

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

    FFINSTANCE()->setFormatContext(fmtCtx_);
}

void Muxer::cleanup() {
    if (fmtCtx_) {
        avformat_free_context(fmtCtx_);
        fmtCtx_ = nullptr;
    }
}

void Muxer::run(std::stop_token st) {
    int res = avformat_write_header(fmtCtx_, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avformat_write_header failed error: %s\n", av_err2str(res));
        ::exit(1);
    }

    while (!st.stop_requested()) {
        AVPacket* pkt = getPacket();
        if (!pkt) {
            msleep(1);
            continue;
        }

        int res = av_interleaved_write_frame(fmtCtx_, pkt);
        if (res < 0) {
            av_log(nullptr, AV_LOG_ERROR, "av_interleaved_write_frame failed error: %s\n", av_err2str(res));
            break;
        }

        pktPool_.put(pkt, false);
    }

    // 线程结束
    res = av_write_trailer(fmtCtx_);
    if (res != 0) {
        av_log(nullptr, AV_LOG_ERROR, "av_write_trailer failed\n");
        ::exit(1);
    }

    res = avio_closep(&fmtCtx_->pb);
    if (res != 0) {
        av_log(nullptr, AV_LOG_ERROR, "avio_closep failed\n");
        ::exit(1);
    }

    av_log(nullptr, AV_LOG_INFO, "muxer thread end\n");
}

void Muxer::close() {
    videoPktQue_.close();
    audioPktQue_.close();
}

void Muxer::handle(void* data) {
    AVPacket* pkt = (AVPacket*)(data);
    AVPacket* pushPkt =  pktPool_.get();
    av_packet_ref(pushPkt, pkt);

    if (pkt->stream_index == VIDEO_STREAM_INDEX) {
        videoPktQue_.push(pushPkt);
    }
    if (pkt->stream_index == AUDIO_STREAM_INDEX) {
        audioPktQue_.push(pushPkt);
    }
}

AVPacket* Muxer::getPacket() {
    if (videoPktQue_.empty() && audioPktQue_.empty()) {
        return nullptr;
    }
    if (!videoPktQue_.empty() && audioPktQue_.empty()) {
        return *videoPktQue_.pop();
    }
    if (videoPktQue_.empty() && !audioPktQue_.empty()) {
        return *audioPktQue_.pop();
    }

    AVRational videoTB = fmtCtx_->streams[VIDEO_STREAM_INDEX]->time_base;
    AVRational audioTB = fmtCtx_->streams[AUDIO_STREAM_INDEX]->time_base;
    int64_t videoTime = 0;
    int64_t audioTime = 0;
    videoPktQue_.visit_front([&](const AVPacket* frame) {
        videoTime = av_rescale_q(frame->pts, videoTB, AV_TIME_BASE_Q);
    });
    audioPktQue_.visit_front([&](const AVPacket* frame) {
        audioTime = av_rescale_q(frame->pts, audioTB, AV_TIME_BASE_Q);
    });

    // 谁少谁优先处理
    if (videoTime < audioTime) {
        return *videoPktQue_.pop();
    }
    else {
        return *audioPktQue_.pop();
    }

    return *videoPktQue_.pop();
}
