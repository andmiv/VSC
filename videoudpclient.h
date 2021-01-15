#ifndef VIDEOUDPCLIENT_H
#define VIDEOUDPCLIENT_H

#include "ivideoupdsocket.h"
#include "datagram.h"
#include "videoclientsettings.h"

#include <QtConcurrent/QtConcurrent>

/*!
 * \class VideoUdpClient
 *
 * \brief представляет класс клиента видеопередачи.
 */

class QTimer;
class VideoStreamController;

class VideoUdpClient : public IVideoUpdSocket
{
    Q_OBJECT
public:
    explicit VideoUdpClient(QObject *parent = Q_NULLPTR);
    virtual ~VideoUdpClient();

    //! завершение передачи и видеозахвата
    void stopBroadcast();
    //! инициализация передачи по адресу addr и начало видеозахвата
    bool startBroadcast(QString addr);

    VideoStreamController *streamController();
    void setStreamController(VideoStreamController *controller);

    VideoClientSettings &settings();
    void setSettings(const VideoClientSettings &settings);

signals:
    //! инициализация сессии передачи (эммитируется, чтобы не обращаться к методам класса из другого потока)
    void send();
    //! ошибка errString отправки кадра
    void transmitionError(QString errString);
    //! Содинение установлено
    void connectionEstablished(QString addr, quint16 port);

private:
    //! остановка передачи кадра
    void endFrameTransmit();
    //! инициализация сессии передачи
    void initFrameTransmit();
    //! сессия передачи
    void sendDatagram();    

    void getDatagram();
    void saveDatagram();
    void connected();

private:
    //! Массив пакетов для передачи
    QFuture<Datagram> m_datagramFuture;
    QFutureWatcher<Datagram> *m_datagramWatcher;
    Datagram m_datagram;
    //! таймер передачи кадров
    QTimer *m_transmittingTimer;

    VideoStreamController *m_streamController;

    VideoClientSettings m_settings;
};

#endif // VIDEOUDPCLIENT_H
