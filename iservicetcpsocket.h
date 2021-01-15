#ifndef SERVICETCPSOCKET_H
#define SERVICETCPSOCKET_H

#include <QObject>
#include <QAbstractSocket>
#include <QMutex>

/*! \class IServiceTcpSocket общий класс для tcp клиентов и серверов */

class QTcpSocket;
class QTcpServer;

class IServiceTcpSocket: public QObject
{
    Q_OBJECT
public:
    explicit IServiceTcpSocket(QObject *parent = Q_NULLPTR);
    virtual ~IServiceTcpSocket();

    virtual quint16 port() const;
    virtual void setPort(const quint16 &port);

protected:
    //! Розорвать текущее соединение
    virtual void closeConnection() = 0;

private:
    quint16 m_port;
};

#endif // SERVICETCPSOCKET_H
