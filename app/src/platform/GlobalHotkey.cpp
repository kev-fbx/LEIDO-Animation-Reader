#include "platform/GlobalHotkey.h"

#include <QCoreApplication>
#include <QLoggingCategory>

#include <windows.h>

Q_LOGGING_CATEGORY(lcHotkey, "pz.hotkey")

GlobalHotkey::GlobalHotkey(unsigned virtualKey, QObject *parent)
    : QObject(parent)
{
    // A null window posts WM_HOTKEY to this (the GUI) thread's message queue.
    m_registered = RegisterHotKey(nullptr, kHotkeyId, MOD_NOREPEAT, virtualKey);
    if (m_registered) {
        QCoreApplication::instance()->installNativeEventFilter(this);
    } else {
        qCWarning(lcHotkey) << "Could not register hotkey" << Qt::hex << virtualKey
                            << "- another program may own it (error" << Qt::dec << GetLastError() << ")";
    }
}

GlobalHotkey::~GlobalHotkey()
{
    if (m_registered) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
        UnregisterHotKey(nullptr, kHotkeyId);
    }
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    // Thread messages arrive as "windows_dispatcher_MSG", window messages as "windows_generic_MSG".
    if (eventType != "windows_dispatcher_MSG" && eventType != "windows_generic_MSG")
        return false;
    const MSG *msg = static_cast<const MSG *>(message);
    if (msg->message == WM_HOTKEY && msg->wParam == WPARAM(kHotkeyId)) {
        emit activated();
        return true;
    }
    return false;
}
