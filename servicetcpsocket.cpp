#include "servicetcpsocket.h"

#include <QTcpServer>
#include <QTcpSocket>

ServiceTcpSocket::ServiceTcpSocket(QObject *parent)
    : QObject(parent)
    , m_localPort(0)
    , m_client(new QTcpSocket)
    , m_server(new QTcpServer)
    , m_serverConnection(nullptr)
    , m_messageToSend()
{
    connect(m_server, &QTcpServer::acceptError, this, &ServiceTcpSocket::getServerError);
    connect(m_client, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(getSocketError(QAbstractSocket::SocketError)));

    connect(m_server, &QTcpServer::newConnection, this, &ServiceTcpSocket::acceptIncomingConnection);
    connect(m_client, &QTcpSocket::connected, this, &ServiceTcpSocket::connected);
}

void ServiceTcpSocket::acceptIncomingConnection()
{
    if(m_serverConnection) {
        disconnect(m_serverConnection, &QTcpSocket::readyRead, this, &ServiceTcpSocket::reseiveMessage);
        disconnect(m_serverConnection, SIGNAL(error(QAbstractSocket::SocketError)),
                   this, SLOT(getSocketError(QAbstractSocket::SocketError)));
        m_serverConnection->close();
        delete m_serverConnection;
        m_serverConnection = nullptr;
    }

    if (!m_server->isListening() && !m_server->listen(QHostAddress::AnyIPv4, m_localPort)) {
        QString err = QString("Unable to start the test: %1.").arg(m_server->errorString());
        emit errorMsg(err);
        return;
    }

    m_serverConnection = m_server->nextPendingConnection();
    connect(m_serverConnection, &QTcpSocket::readyRead, this, &ServiceTcpSocket::reseiveMessage);
    connect(m_serverConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(getSocketError(QAbstractSocket::SocketError)));
    m_serverConnection->open(QTcpSocket::ReadOnly);
}

void ServiceTcpSocket::sendMessageToHost(QByteArray message, QHostAddress addr, quint16 port)
{
    m_client->connectToHost(addr, port);
    connect(m_client, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(getSocketError(QAbstractSocket::SocketError)));
    m_client->open(QTcpSocket::WriteOnly);
    m_messageToSend = message;
}

void ServiceTcpSocket::setLocalPort(quint16 localPort)
{
    if(localPort == m_localPort)
        return;

    m_localPort = localPort;
    m_server->listen(QHostAddress::AnyIPv4, m_localPort);
}

void ServiceTcpSocket::connected()
{
    m_client->write(m_messageToSend);
    m_messageToSend.clear();
}

void ServiceTcpSocket::reseiveMessage()
{
    emit messageReseived(m_serverConnection->readAll(),
                         m_serverConnection->peerAddress());
}

void ServiceTcpSocket::getSocketError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    emit errorMsg(dynamic_cast<QTcpSocket *>(sender())->errorString());
}

void ServiceTcpSocket::getServerError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    emit errorMsg(dynamic_cast<QTcpServer *>(sender())->errorString());
}
