#ifndef SERVICETCPCLIENT_H
#define SERVICETCPCLIENT_H

#include "iservicetcpsocket.h"

/*! \class ServiceTcpClient - реализация tcp клиента
 * Если объект класса существует в другом потоке, вызов любого метода вне класса должен
 * происходить эмиттированием сигнала, на который будет подписан вызываемый
 *
 * Сигнал connectionFaild эммитируется по сигналу от таймера m_connectionTimer,
 * когда время ожидания превысило допустимое. Максимальное время ожидания соединение
 * может быть установлено пользователем.
*/

class QTcpSocket;
class QTimer;

class ServiceTcpClient : public IServiceTcpSocket
{
    Q_OBJECT
public:
    explicit ServiceTcpClient(QObject *parent = Q_NULLPTR);
    virtual ~ServiceTcpClient();
    //! Отправить сообщение message по адресу addr
    void sendMessageToHost(QByteArray message,
                           QString addr);

    void setConnectionTimeout(quint16 timeout);

private:
    //! Непосредственная отправка сообщения
    void send();
    //! Закрытие соединения
    void closeConnection();

signals:
    //! Ошибка подключения
    void connectionFaild();

private:
    //! Сокет клиента
    QTcpSocket *m_socket;
    //! Таймет на максимальное время ожидания подключения
    QTimer *m_connectionTimer;
    //! Сообщение для отправки
    QByteArray m_messageToSend;
    //! Максимальное время ожидания соединения
    quint16 m_connectionTimeout;
};

#endif // SERVICETCPCLIENT_H
