//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/recorderController.h>
#include <DreamRecorder/DreamRecorderContext.h>
#include <recorderComp/recorderWorker.h>
#include <recorderComp/cameraCapture.h>
#include <recorderComp/audioCapture.h>

#include <QSize>
#include <QDebug>
#include <QThread>

using namespace Dream;

namespace {
    constexpr uint32_t FPS = 30;

    // 映射
    SampleFormat toDreamSampleFormat(QAudioFormat::SampleFormat format) {
        switch (format) {
            case QAudioFormat::UInt8: return SampleFormat::U8;
            case QAudioFormat::Int16: return SampleFormat::S16;
            case QAudioFormat::Int32: return SampleFormat::S32;
            case QAudioFormat::Float: return SampleFormat::FLT;
            default:
                qWarning() << "unsupported sample format";
                return SampleFormat::NONE;
        }
    }
}

void RecorderController::start(const QString& path) {
    cameraCapture_ = new CameraCapture(this, FPS);
    audioCapture_ = new AudioCapture(this);
    const QSize size = cameraCapture_->getCameraResolution();
    VideoParam videoParam{};
    videoParam.width = size.width();
    videoParam.height = size.height();
    videoParam.fps = FPS;

    AudioParam audioParam{};
    const QAudioFormat audioForamt = audioCapture_->getAudioFormat();
    audioParam.sampleRate = audioForamt.sampleRate();
    audioParam.channels = audioForamt.channelCount();
    audioParam.sampleFormat = toDreamSampleFormat(audioForamt.sampleFormat());

    recorder_ = DreamRecorderContext::createDreamRecorder();
    recorder_->start(path.toLocal8Bit(), videoParam, audioParam);

    videoThread_ = new QThread(this);
    videoWorker_ = new VideoWorker(recorder_, FPS);
    videoWorker_->moveToThread(videoThread_);
    connect(cameraCapture_, &CameraCapture::imageCaptured, videoWorker_, &VideoWorker::onImageCaptured, Qt::QueuedConnection);
    connect(videoThread_, &QThread::finished, videoWorker_, &QObject::deleteLater);
    videoThread_->start();

    audioThread_ = new QThread(this);
    audioWorker_ = new AudioWorker(recorder_);
    audioWorker_->moveToThread(audioThread_);
    connect(audioCapture_, &AudioCapture::audioDataCaptured, audioWorker_, &AudioWorker::onAudioDataCaptured, Qt::QueuedConnection);
    connect(audioThread_, &QThread::finished, audioWorker_, &QObject::deleteLater);
    audioThread_->start();
}

void RecorderController::stop() {
    if (cameraCapture_) {
        delete cameraCapture_;
        cameraCapture_ = nullptr;
    }

    if (videoThread_) {
        videoThread_->exit();
        videoThread_->wait();
        delete videoThread_;
        videoThread_ = nullptr;
    }

    if (audioCapture_) {
        delete audioCapture_;
        audioCapture_ = nullptr;
    }

    if (audioThread_) {
        audioThread_->exit();
        audioThread_->wait();
        delete audioThread_;
        audioThread_ = nullptr;
    }

    if (recorder_) {
        recorder_->stop();
        DreamRecorderContext::destroyDreamRecorder(&recorder_);
    }
}
