#include "servicetcpclient.h"

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

ServiceTcpClient::ServiceTcpClient(QObject *parent)
    : IServiceTcpSocket(parent)
    , m_socket(new QTcpSocket(this))
    , m_connectionTimer(new QTimer(this))
    , m_messageToSend()
    , m_connectionTimeout(3000)
{
    connect(m_socket, &QTcpSocket::connected, this, &ServiceTcpClient::send);

    connect(m_connectionTimer, &QTimer::timeout, this, &ServiceTcpClient::connectionFaild);
    connect(this, &ServiceTcpClient::connectionFaild, this, &ServiceTcpClient::closeConnection);
}

ServiceTcpClient::~ServiceTcpClient()
{
    disconnect(m_socket, &QTcpSocket::connected, this, &ServiceTcpClient::send);
    disconnect(this, &ServiceTcpClient::connectionFaild, this, &ServiceTcpClient::closeConnection);
    disconnect(m_connectionTimer, &QTimer::timeout, this, &ServiceTcpClient::connectionFaild);
    closeConnection();
}

void ServiceTcpClient::sendMessageToHost(QByteArray message,
                                         QString addr)
{
    m_messageToSend = message;

    m_socket->connectToHost(addr, port());
    m_socket->open(QTcpSocket::WriteOnly);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->start(m_connectionTimeout);
}

void ServiceTcpClient::setConnectionTimeout(quint16 timeout)
{
    if(m_connectionTimeout)
        return;
    m_connectionTimeout = timeout;
}

void ServiceTcpClient::send()
{
    m_connectionTimer->stop();
    m_socket->write(m_messageToSend);
    closeConnection();
}

void ServiceTcpClient::closeConnection()
{
    m_messageToSend.clear();
    m_socket->disconnectFromHost();
    m_socket->close();
}
