//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <recorderComp/recorderController.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QMetaType>

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

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("config");
    QGuiApplication::setApplicationName("recorder");

    qmlRegisterType<RecorderController>("Dream.recorder", 1, 0, "RecorderController");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Dream.recorder", "Main");

    return QCoreApplication::exec();
}
