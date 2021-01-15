#ifndef STREAMWINDOW_H
#define STREAMWINDOW_H

#include <QQuickPaintedItem>
#include <QImage>
#include <QHostAddress>

/*! \class VideoStreamer реализует управление видеопотоком*/

/*
 * TODO все настроики не должны наследоваться от QObject
 *      пересмотреть наследование от QObject, в каких то классах оно не нежно
 */

class SocketThread;
class ServiceTcpClient;
class ServiceTcpServer;
class VideoUdpClient;
class VideoUdpServer;

class VideoStreamer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(ConnectionStatus status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QByteArray frameFormat READ frameFormat WRITE setFrameFormat NOTIFY frameFormatChanged)
    Q_PROPERTY(quint16 acceptTime READ acceptTime WRITE setAcceptTime NOTIFY acceptTimeChanged)


public:
    enum ServiceMessage
    {
        //! Запрос на передачу трансляции
        TransmittingRequest                 = 0,
        //! Запрос на прием трансляции
        ReseivingRequest                    = 1 << 1,
        //! Принять запрос на передачу
        TransmitRequestAccepted             = 1 << 2,
        //! Отклонить запрос на передачу
        TransmitRequestDeny                 = 1 << 3,
        //! Принять запрос на прием
        ReceiveRequestAccepted              = 1 << 4,
        //! Отклонить запрос на прием
        ReceiveRequestDeny                 = 1 << 5,
        //! Передача остановлена
        TransmittingStopped                 = 1 << 6,
        //! Прием остановлен
        ReceivingStopped                    = 1 << 7
    };
    Q_ENUM(ServiceMessage)

    enum ConnectionStatus
    {
        //! Соединение не установлено
        NotConnected                    = 0,
        //! Исходящее соединение установлено
        OutgoingConnection              = 1 << 1,
        //! Идет исходящая трансляция
        Transmitting                    = 1 << 2,
        //! Ошибка при попытке установить соединение
        ConnectionFaild                 = 1 << 3,
        //! Входящий запрос на прием
        IncomingReseiveRequest          = 1 << 4,
        //! Входящий запрос на передачу
        IncomingTransmitRequest         = 1 << 5,
        //! Идет входящая трансляция
        Receiving                       = 1 << 6
    };
    Q_ENUM(ConnectionStatus)

    explicit VideoStreamer(QQuickItem *parent = Q_NULLPTR);
    ~VideoStreamer();
    //! геттеры
    ConnectionStatus status() const;
    quint16 port() const;
    QByteArray frameFormat() const;
    QImage lastFrame() const;
    quint16 acceptTime() const;

    virtual void paint(QPainter *painter) override;

public slots:
    //! сеттеры
    void setStatus(ConnectionStatus status);
    void setPort(quint16 port);
    void setFrameFormat(QByteArray frameFormat);
    void setAcceptTime(quint16 acceptTime);
    //! Запрос исходящей трансляции
    void transmittingRequest(QString addr);
    //! Запрос входящей трансляции
    void receivingRequest(QString addr);
    //! Принять запрос на передачу
    void acceptTransmitRequest();
    //! Отклонить запрос на передачу
    void denyTransmitRequest();
    //! Принять запрос на прием
    void acceptReceiveRequest();
    //! Отклонить запрос на прием
    void denyReceiveRequest();
    //! Остановить входящюю трансляцию
    void stopTransmitting();
    //! Остановить исходящую трансляцию
    void stopReceiving();

private slots:
    //! Получено новое сообщение
    void onMessageReceived(QByteArray msg, QString addr);
    //! Получен новый кадр
    void onFrameReceived();

signals:
    void statusChanged(ConnectionStatus status);
    void portChanged(quint16 port);
    void frameFormatChanged(QByteArray frameFormat);
    void acceptTimeChanged(quint16 acceptTime);
    //! начать исходящую трансляцию
    void beginTransmitting(QString addr);
    //! принять входящую трансляцию
    void acceptStream();
    //! закончить передачу
    void endTransmitting();
    //! закончить прием
    void endReseiving();
    //! отравить сообщение
    void sendMessage(QByteArray msg, QString addr);

private:
    //! Последний принятый кадр
    QImage m_lastFrame;
    //! Потоки в которых работают сокеты и захватываются кадры
    SocketThread *m_serviceThread;
    SocketThread *m_videoThread;
    //! Сервесные tcp сокеты
    ServiceTcpClient *m_serviceClient;
    ServiceTcpServer *m_serviceServer;
    //! Udp сокеты видеопотока
    VideoUdpClient *m_videoClient;
    VideoUdpServer *m_videoServer;
    //! Адрес tcp соединения
    QHostAddress m_tcpAddress;
    //! Адрес upd соединения
    QHostAddress m_udpAddress;
    //! Статус соединения
    ConnectionStatus m_status;
    //! Таймер на принятие решения
    QTimer *m_acceptTimer;
    //! порт для tcp и udp
    quint16 m_port;
    //! Формат передаваемого кадра
    QByteArray m_frameFormat;
    //! Переменная таймера принятия решения (для отображения пользователю)
    quint16 m_acceptTime;
};

#endif // STREAMWINDOW_H
