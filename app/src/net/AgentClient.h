#pragma once

#include "protocol/Protocol.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

// Connects to the in-game agent, keeps reconnecting while the game is closed, and turns the byte
// stream into HELLO / SKELETON / FRAME / STATUS updates.
class AgentClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString gameVersion READ gameVersion NOTIFY helloChanged)
    Q_PROPERTY(bool supportedBuild READ supportedBuild NOTIFY helloChanged)
    Q_PROPERTY(bool recording READ isRecording NOTIFY statusChanged)
    Q_PROPERTY(QString lastTake READ lastTake NOTIFY statusChanged)
    Q_PROPERTY(qint64 framesWritten READ framesWritten NOTIFY statusChanged)
    Q_PROPERTY(double fps READ fps NOTIFY statsChanged)
    Q_PROPERTY(QVector3D position READ position NOTIFY statsChanged)
    Q_PROPERTY(quint16 port READ port CONSTANT)

public:
    explicit AgentClient(quint16 port = pz::kDefaultPort, QObject *parent = nullptr);

    bool isConnected() const { return m_socket.state() == QAbstractSocket::ConnectedState; }
    QString gameVersion() const { return m_hello.gameVersion; }
    bool supportedBuild() const { return m_hello.supported; }
    bool isRecording() const { return m_recording; }
    QString lastTake() const { return m_lastTake; }
    qint64 framesWritten() const { return m_framesWritten; }
    double fps() const { return m_fps; }
    QVector3D position() const { return m_position; }
    quint16 port() const { return m_port; }

    // dir: folder for the agent's raw .pzrec take (empty = agent default).
    Q_INVOKABLE void startRecording(const QString &dir);
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void toggleRecording(const QString &dir);

signals:
    void connectedChanged();
    void helloChanged();
    void statusChanged();
    void statsChanged();
    void skeletonReceived(const pz::Skeleton &skeleton);
    void frameReceived(const pz::Frame &frame);

private:
    void tryConnect();
    void onReadyRead();
    void handleMessage(pz::MsgType type, const QByteArray &payload);
    void handleStatus(const QByteArray &json);
    void updateStats();

    const quint16 m_port;
    QTcpSocket m_socket;
    QTimer m_reconnectTimer;
    QTimer m_statsTimer;
    QByteArray m_buffer;

    pz::Hello m_hello;
    bool m_recording = false;
    QString m_lastTake;
    qint64 m_framesWritten = 0;
    int m_framesThisSecond = 0;
    double m_fps = 0;
    QVector3D m_position;
};
