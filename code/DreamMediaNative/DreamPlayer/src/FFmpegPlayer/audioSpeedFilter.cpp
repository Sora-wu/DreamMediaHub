//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioSpeedFilter.h>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

AudioSpeedFilter::~AudioSpeedFilter() {
    if (graph_) {
        avfilter_graph_free(&graph_);
        graph_ = nullptr;
    }
}

int AudioSpeedFilter::initFilter(float speed, uint32_t sampleRate, AVSampleFormat sampleFmt, uint64_t channelLayout) {
    const AVFilter* abufferSrcFilter = avfilter_get_by_name("abuffer");
    const AVFilter* abufferSinkFilter = avfilter_get_by_name("abuffersink");
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    graph_ = avfilter_graph_alloc();
    if (!outputs || !inputs || !graph_) {
        return AVERROR(ENOMEM);
    }

    char args[512]{};
    snprintf(args, sizeof(args),
             "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             sampleRate, sampleRate, av_get_sample_fmt_name(sampleFmt), channelLayout);
    int ret = 0;
    do {
        ret = avfilter_graph_create_filter(&filterSrcCtx_, abufferSrcFilter, "in", args, nullptr, graph_);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to parse filter graph\n");
            break;
        }

        ret = avfilter_graph_create_filter(&filterSinkCtx_, abufferSinkFilter, "out", nullptr, nullptr, graph_);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to create audio buffer sink\n");
            break;
        }

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = filterSrcCtx_;
        outputs->pad_idx    = 0;
        outputs->next       = nullptr;

        inputs->name       = av_strdup("out");
        inputs->filter_ctx = filterSinkCtx_;
        inputs->pad_idx    = 0;
        inputs->next       = nullptr;

        char filterDesc[128]{};
        snprintf(filterDesc, sizeof(filterDesc), "atempo=%.2f", speed);
        ret = avfilter_graph_parse_ptr(graph_, filterDesc, &inputs, &outputs, nullptr);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to parse filter graph\n");
            break;
        }

        ret = avfilter_graph_config(graph_, nullptr);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to configure filter graph\n");
            break;
        }
    } while (false);

    if (ret < 0) {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    }
    return ret;
}

int AudioSpeedFilter::addFrame(AVFrame* frame) {
    return av_buffersrc_add_frame_flags(filterSrcCtx_, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
}

int AudioSpeedFilter::getFrame(AVFrame* frame) {
    return av_buffersink_get_frame(filterSinkCtx_, frame);
}
