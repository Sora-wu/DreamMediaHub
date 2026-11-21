//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <common/define.h>
#include <videoComp/videoArea.h>
#include <videoComp/audioBufferDevice.h>
#include <videoComp/playerController.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        qFatal() << "Usage: playerApp <video_file>";
    }

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(PLAYER_OPENGL_VERSION_MAJOR, PLAYER_OPENGL_VERSION_MINOR);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    qmlRegisterType<VideoArea>("Dream.play", 1, 0, "VideoArea");
    qmlRegisterType<PlayerController>("Dream.play", 1, 0, "PlayerController");

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dream.play", "Main");

    return QCoreApplication::exec();
}
