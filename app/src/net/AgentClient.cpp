#include "net/AgentClient.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QtEndian>

Q_LOGGING_CATEGORY(lcAgent, "pz.agent")

AgentClient::AgentClient(quint16 port, QObject *parent)
    : QObject(parent)
    , m_port(port)
{
    m_socket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        qCInfo(lcAgent) << "Connected to agent on port" << m_port;
        m_buffer.clear();
        emit connectedChanged();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        qCInfo(lcAgent) << "Agent disconnected";
        m_hello = {};
        m_recording = false;
        emit connectedChanged();
        emit helloChanged();
        emit statusChanged();
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, &AgentClient::onReadyRead);

    // The game may start after the app; keep knocking once a second.
    m_reconnectTimer.setInterval(1000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &AgentClient::tryConnect);
    m_reconnectTimer.start();
    tryConnect();

    m_statsTimer.setInterval(1000);
    connect(&m_statsTimer, &QTimer::timeout, this, &AgentClient::updateStats);
    m_statsTimer.start();
}

void AgentClient::startRecording(const QString &dir)
{
    if (!isConnected())
        return;
    QJsonObject request;
    if (!dir.isEmpty())
        request.insert("dir", dir);
    m_socket.write(pz::encodeMessage(pz::MsgType::RecordStart, QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void AgentClient::stopRecording()
{
    if (isConnected())
        m_socket.write(pz::encodeMessage(pz::MsgType::RecordStop));
}

void AgentClient::toggleRecording(const QString &dir)
{
    if (m_recording)
        stopRecording();
    else
        startRecording(dir);
}

void AgentClient::tryConnect()
{
    if (m_socket.state() == QAbstractSocket::UnconnectedState)
        m_socket.connectToHost(QHostAddress::LocalHost, m_port);
}

void AgentClient::onReadyRead()
{
    m_buffer.append(m_socket.readAll());
    qsizetype offset = 0;
    while (m_buffer.size() - offset >= 4) {
        const quint32 length = qFromLittleEndian<quint32>(m_buffer.constData() + offset);
        if (length < 1 || length > pz::kMaxMessageBytes) {
            qCWarning(lcAgent) << "Bad message length" << length << "- dropping connection";
            m_socket.abort();
            m_buffer.clear();
            return;
        }
        if (m_buffer.size() - offset < 4 + qsizetype(length))
            break;
        const auto type = pz::MsgType(quint8(m_buffer.at(offset + 4)));
        handleMessage(type, m_buffer.mid(offset + 5, length - 1));
        offset += 4 + length;
    }
    m_buffer.remove(0, offset);
}

void AgentClient::handleMessage(pz::MsgType type, const QByteArray &payload)
{
    switch (type) {
    case pz::MsgType::Hello:
        if (auto hello = pz::Hello::fromJson(payload)) {
            if (hello->protocol != pz::kProtocolVersion)
                qCWarning(lcAgent) << "Agent speaks protocol" << hello->protocol << "- expected" << pz::kProtocolVersion;
            m_hello = *hello;
            emit helloChanged();
        }
        break;
    case pz::MsgType::Skeleton:
        if (auto skeleton = pz::Skeleton::fromJson(payload)) {
            qCInfo(lcAgent) << "Skeleton" << skeleton->id << "with" << skeleton->bones.size() << "bones";
            emit skeletonReceived(*skeleton);
        }
        break;
    case pz::MsgType::Frame:
        if (auto frame = pz::Frame::parse(payload)) {
            ++m_framesThisSecond;
            m_position = frame->position;
            emit frameReceived(*frame);
        }
        break;
    case pz::MsgType::Status:
        handleStatus(payload);
        break;
    default:
        break; // Reserved types (STATE, ITEMS, OBSTACLES) are ignored until implemented.
    }
}

void AgentClient::handleStatus(const QByteArray &json)
{
    const QJsonObject status = QJsonDocument::fromJson(json).object();
    m_recording = status.value("recording").toBool();
    m_lastTake = status.value("take").toString();
    m_framesWritten = status.value("framesWritten").toInteger();
    emit statusChanged();
}

void AgentClient::updateStats()
{
    m_fps = m_framesThisSecond;
    m_framesThisSecond = 0;
    emit statsChanged();
}
