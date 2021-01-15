#ifndef VIDEOUPDSOCKET_H
#define VIDEOUPDSOCKET_H

#include <QObject>
#include <QMutex>

/*! \class IVideoUdpSocket общий класс для udp клиентов и серверов */

class QUdpSocket;

class IVideoUpdSocket: public QObject
{
    Q_OBJECT
public:
    explicit IVideoUpdSocket(QObject *parent = Q_NULLPTR);
    virtual ~IVideoUpdSocket();

    //! геттеры
    virtual quint16 port() const;
    virtual bool isProcessing() const;
    virtual QUdpSocket *socket() const;
    //! сеттеры
    virtual void setPort(const quint16 &port);
    //! остановка трансляции
    virtual void stopBroadcast() = 0;

protected:
    //! Сеттер защищен, так как нельзя вне класса устанавливать поле m_isPorcessing
    virtual void setIsProcessing(bool processing);

private:
    //! идет ли трансляция
    bool m_isProcessing;
    //! текущий порт
    quint16 m_port;
    //! сокет клиента(сервера) (есть только геттер, т.к. после создания сокет больше менять нельзя)
    QUdpSocket *m_socket;
};

#endif // VIDEOUPDSOCKET_H
