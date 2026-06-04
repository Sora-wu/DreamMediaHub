//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <videoComp/videoArea.h>
#include <videoComp/audioBufferPlayer.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QMediaDevices>
#include <QAudioSink>

#include <DreamPlayer/DreamContext.h>

using namespace Dream;

namespace {
    class PlayerAPP {
    public:
        PlayerAPP() {
            player_ = DreamContext::createDreamPlayer();

            QAudioFormat format;
            format.setSampleRate(AudioBufferDevice::SAMPLE_RATE);
            format.setChannelCount(AudioBufferDevice::CHANNEL_COUNT);
            format.setSampleFormat(QAudioFormat::Int16);
            QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
            if (!audioDevice.isFormatSupported(format)) {
                qInfo() << "sample rate: " << format.sampleRate();
                qInfo() << "channel count: " << format.channelCount();
                qWarning() << "can not support this audio format or detect any audio device, will play with no audio";
            }
            audioBufferDevice_ = new AudioBufferDevice;
            audioSink_ = new QAudioSink(audioDevice, format);
        }

        ~PlayerAPP() {
            delete audioBufferDevice_;
            delete audioSink_;

            DreamContext::destroyDreamPlayer(&player_);
        }

        void initPlayer(VideoArea* videoArea) {
            // TEST
#ifdef PLATFORM_LINUX
            player_->open("/home/sora/Videos/test.mp4");
#else
            player_->open("D:/Desktop/Test/test31.mp4");
#endif

            videoArea->setPlayer(player_);
            audioBufferDevice_->setPlayer(player_);

            audioBufferDevice_->startPull();
            // 预缓存
            if (!audioBufferDevice_->waitForBuffered(std::chrono::milliseconds(800))) {
                qWarning() << "prebuffer timeout";
            }
            audioSink_->setBufferSize(DreamContext::AUDIO_BUFFER_SIZE);
            audioSink_->start(audioBufferDevice_);
        }

    private:
        AudioBufferDevice* audioBufferDevice_ = nullptr;
        QAudioSink* audioSink_ = nullptr;
        DreamPlayer* player_ = nullptr;
    };
}

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    qmlRegisterType<VideoArea>("Dream.play", 1, 0, "VideoArea");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dream.play", "Main");

    QObject* rootObject = engine.rootObjects().first();
    VideoArea* videoArea = rootObject->findChild<VideoArea*>("videoArea");
    PlayerAPP playerApp{};
    if (videoArea) {
        playerApp.initPlayer(videoArea);
    }

    return QCoreApplication::exec();
}
