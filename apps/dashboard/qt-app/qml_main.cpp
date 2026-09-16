#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "QtDashboardBackend.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QtDashboardBackend backend;

    engine.rootContext()->setContextProperty(
        "dashboardBackend",
        &backend);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/CoolingDashboard/qml/Main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}