#include "servicetcpserver.h"

#include <QTcpServer>
#include <QTcpSocket>

ServiceTcpServer::ServiceTcpServer(QObject *parent)
    : IServiceTcpSocket(parent)
    , m_server(new QTcpServer(this))
    , m_serverSocket(Q_NULLPTR)
{
    connect(m_server, &QTcpServer::newConnection, this, &ServiceTcpServer::acceptIncomingConnection);
    m_server->listen(QHostAddress::AnyIPv4, this->port());
}

ServiceTcpServer::~ServiceTcpServer()
{
    disconnect(m_server, &QTcpServer::newConnection, this, &ServiceTcpServer::acceptIncomingConnection);
    closeConnection();
    delete m_server;
    delete m_serverSocket;
}

void ServiceTcpServer::setPort(const quint16 &port)
{
    IServiceTcpSocket::setPort(port);
    m_server->close();
    m_server->listen(QHostAddress::AnyIPv4, this->port());
}

void ServiceTcpServer::acceptIncomingConnection()
{
    if(m_serverSocket) closeConnection();

    if (!m_server->isListening() && !m_server->listen(QHostAddress::AnyIPv4, port())) {
        QString err = QString("Unable to start the test: %1.").arg(m_server->errorString());
        qDebug() << err;
        return;
    }

    m_serverSocket = m_server->nextPendingConnection();
    connect(m_serverSocket, &QTcpSocket::readyRead, this, &ServiceTcpServer::reseiveMessage);
    m_serverSocket->open(QTcpSocket::ReadOnly);
}

void ServiceTcpServer::closeConnection()
{
    disconnect(m_serverSocket, &QTcpSocket::readyRead, this, &ServiceTcpServer::reseiveMessage);
    m_serverSocket->close();
}

void ServiceTcpServer::reseiveMessage()
{
    QByteArray message;
    message.resize(m_serverSocket->bytesAvailable());
    message = m_serverSocket->readAll();
    emit messageReceived(message, m_serverSocket->peerAddress().toString());
}
