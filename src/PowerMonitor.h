#pragma once
#include <QAbstractNativeEventFilter>
class AppController;
class PowerMonitor final : public QAbstractNativeEventFilter {
public:
    explicit PowerMonitor(AppController *controller) : m_controller(controller) {}
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
private:
    AppController *m_controller;
};

