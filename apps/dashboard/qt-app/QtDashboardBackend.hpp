#pragma once

#include <QObject>

#include "../DashboardBackend.hpp"

/// @brief Qt/QML adapter for the existing DashboardBackend.
///
/// @details
/// Exposes the non-Qt DashboardBackend to the QML user interface
/// through Qt signals and invokable functions.
class QtDashboardBackend : public QObject
{
    Q_OBJECT

public:
    explicit QtDashboardBackend(QObject *parent = nullptr);

    /// @brief Initializes the dashboard backend.
    /// @return true if initialization succeeds.
    Q_INVOKABLE bool Init();

    /// @brief Starts CAN processing and control execution.
    Q_INVOKABLE void Start();

    /// @brief Waits for backend threads to finish.
    Q_INVOKABLE void Join();

signals:
    /// @brief Reports dashboard values to QML.
    void valuesChanged(int rpm,
                       int temperature,
                       int pressure,
                       int voltage,
                       bool running);

private:
    DashboardBackend Backend;
};