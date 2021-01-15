#ifndef SERVICETCPSOCKET_H
#define SERVICETCPSOCKET_H

#include <QObject>
#include <QHostAddress>

/*! \class ServiceTcpSocket позволяет машинам обмениваться сервисными сообщениями по tcp */

class QTcpSocket;
class QTcpServer;

class ServiceTcpSocket: public QObject
{
    Q_OBJECT
public:
    explicit ServiceTcpSocket(QObject *parent = nullptr);

    //отправить сообщение
    void sendMessageToHost(QByteArray message, QHostAddress addr, quint16 port);
    void setLocalPort(quint16 localPort);

signals:
    //отправка и прием сообщений
    void messageReseived(QByteArray message, QHostAddress addr);

    //ошибки и предупреждения
    void errorMsg(QString errMst);

private:
    void reseiveMessage();
    void connected();
    //принять соединение
    void acceptIncomingConnection();

private slots:
    void getSocketError(QAbstractSocket::SocketError err);
    void getServerError(QAbstractSocket::SocketError err);

private:
    quint16 m_localPort;
    QTcpSocket *m_client;
    QTcpServer *m_server;
    QTcpSocket *m_serverConnection;

    QByteArray m_messageToSend;
};

#endif // SERVICETCPSOCKET_H
