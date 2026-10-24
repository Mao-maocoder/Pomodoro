#include "PowerMonitor.h"
#include "AppController.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif
bool PowerMonitor::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
#ifdef Q_OS_WIN
    const auto *nativeMessage = static_cast<MSG *>(message);
    if (nativeMessage->message == WM_POWERBROADCAST && nativeMessage->wParam == PBT_APMSUSPEND)
        m_controller->suspend();
#else
    Q_UNUSED(message);
#endif
    return false;
}

