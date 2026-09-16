#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "QtDashboardBackend.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QtDashboardBackend backend;

    engine.rootContext()->setContextProperty(
        "dashboardBackend",
        &backend);

    engine.loadFromModule("CoolingDashboard", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}