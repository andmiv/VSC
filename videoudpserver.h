#ifndef VIDEOUDPSERVER_H
#define VIDEOUDPSERVER_H

#include <QObject>
#include <QImage>
#include <QtConcurrent/QtConcurrent>

#include "ivideoupdsocket.h"
#include "datagram.h"
#include "videostreamcontroller.h"

/*! \class VideoUdpServer реализует udp сервер
 * Если объект класса существует в другом потоке, вызов любого метода вне класса должен
 * происходить эмиттированием сигнала, на который будет подписан вызываемый
 *
 * Чтобы начать прием трансляции нужно вызвать метод acceptBroadcast,
 * который инициализирует сокет и начнет принимать пакеты
 */

/*
 * TODO
 * Разделить класс на класс выдеозахвати и видеопередачи, а также создать отдельный класс датаграмм (фреймов)
 * для соответствия SOLID
 */

class VideoUdpServer : public IVideoUpdSocket
{
    Q_OBJECT
public:
    explicit VideoUdpServer(QObject *parent = nullptr);
    virtual ~VideoUdpServer();

    //! принять входящее соединение
    void acceptBroadcast();
    //! остановить трансляцию
    void stopBroadcast();

    VideoStreamController *streamContoroller() const;
    void setStreamController(VideoStreamController *controller);

private:
    //! принять пакеты
    void receive();

signals:
    //! кадр получен и собран
    void frameReseived();
    //! передача размеровв экрана для правильной перерисовки кадра
    void sizeChanged(quint16 width, quint16 height);

private:
    //! Ожидаетсся новый кадр
    bool m_readyToReceive;
    //! Контрольная сумма
    quint16  m_checksum;
    //! Хранилище полученных пакетов
    Datagram m_datagram;
    QFuture<void> m_futureFrame;
    QFutureWatcher<void> *m_frameWatcher;

    VideoStreamController *m_streamController;
};

#endif // VIDEOUDPSERVER_H
