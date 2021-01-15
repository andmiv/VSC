#ifndef SERVICETCPSERVER_H
#define SERVICETCPSERVER_H

#include <QObject>
#include <QHostAddress>

#include "iservicetcpsocket.h"

/*! \class ServiceTcpServer - реализация tcp сервера
 * Если объект класса существует в другом потоке, вызов любого метода вне класса должен
 * происходить эмиттированием сигнала, на который будет подписан вызываемый
 *
 * Получение сообщения происходит подпиской на сигнал messageReseived,
 * который передает полученное сообщение и адрес отправтеля
*/

class ServiceTcpServer : public IServiceTcpSocket
{
    Q_OBJECT
public:
    explicit ServiceTcpServer(QObject *parent = Q_NULLPTR);
    virtual ~ServiceTcpServer();

    //! Перегруженный сеттер установки порта, чтобы переподписаться на него
    void setPort(const quint16 &port);

private:
    //! Установть входящее соединение
    void acceptIncomingConnection();
    //! Закрыть соединение
    void closeConnection();
    //! Непосредственное получение сообщения
    void reseiveMessage();

signals:
    //! Сообщение message получено с адреса addr
    void messageReceived(QByteArray message, QString addr);

private:
    //! Сокет сервера
    QTcpServer *m_server;
    //! Сокет входящего соединения
    QTcpSocket *m_serverSocket;
};

#endif // SERVICETCPSERVER_H
