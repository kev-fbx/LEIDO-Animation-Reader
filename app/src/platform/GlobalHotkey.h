#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

// System-wide hotkey (Win32 RegisterHotKey), so Record/Stop works while the game has focus.
// Windows delivers a registered hotkey to us instead of the focused game.
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    // virtualKey: a Win32 VK_* code, e.g. VK_F9 (0x78).
    explicit GlobalHotkey(unsigned virtualKey, QObject *parent = nullptr);
    ~GlobalHotkey() override;

    bool isRegistered() const { return m_registered; }

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void activated();

private:
    static constexpr int kHotkeyId = 0x5A41; // arbitrary, unique within this process
    bool m_registered = false;
};
