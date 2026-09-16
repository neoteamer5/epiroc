#include "QtDashboardBackend.hpp"

QtDashboardBackend::QtDashboardBackend(QObject *parent)
    : QObject(parent)
{
}

bool QtDashboardBackend::Init()
{
    return Backend.Init();
}

void QtDashboardBackend::Start()
{
    Backend.Start();
}

void QtDashboardBackend::Join()
{
    Backend.Join();
}