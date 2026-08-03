//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <videoComp/videoArea.h>
#include <videoComp/audioBufferPlayer.h>
#include <videoComp/playerController.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qCritical() << "Usage: playerApp <video_file>";
    }

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    qmlRegisterType<VideoArea>("Dream.play", 1, 0, "VideoArea");
    qmlRegisterType<PlayerController>("Dream.play", 1, 0, "PlayerController");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dream.play", "Main");

    return QCoreApplication::exec();
}
