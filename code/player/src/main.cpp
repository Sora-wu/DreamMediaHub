//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <render/videoArea.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include <DreamPlayer/DreamContext.h>

void initPlayer() {
    using namespace Dream;

    DreamPlayer* player = DreamContext::createDreamPlayer();
    VideoArea::setPlayer(player);


    // TEST
#ifdef PLATFORM_LINUX
    player->open("/home/sora/Videos/test.mp4");
#else
    player->open("D:/Desktop/Test/test31.mp4");
#endif
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

    initPlayer();

    return QCoreApplication::exec();
}
