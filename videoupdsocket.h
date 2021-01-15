#ifndef VIDEOUPDSOCKET_H
#define VIDEOUPDSOCKET_H

#include <QObject>
#include <QHostAddress>
#include <QApplication>
#include <QImage>

class QTimer;
class QUdpSocket;

class VideoUpdSocket: public QObject
{
    Q_OBJECT
public:
    explicit VideoUpdSocket(const quint16 &localPort,
                            const QByteArray &frameFormat = "jpg",
                            QObject *parent = nullptr);

    void setWidth(int width);
    void setHeight(int height);
    void setFramePartSize(int size);
//    void setFrameFormat(QByteArray frameFormat);
//    void setLocalPort(quint16 port);
    void startBroadcast(QHostAddress addr, WId winId);
    void asseptBreadcast();
    void stopBroadcast();

private:
    void initFrameTransmit();
    void endFrameTransmit();
    void send();
    void cupture();
    void createDatagram(WId winId, int width, int height, int framePartSize);
    void reseive();
    void createFrame(QByteArray frame);

signals:
//    void localPortChanged(quint16 port);
//    void frameFormatChanged(QByteArray frameFormat);
    void framePartSizeChenged(int size);
    void widthChanged(int width);
    void heightChanged(int height);

    void startBroadcastRequest(QHostAddress addr, WId winId);
    void asseptBroadcastRequest();
    void stopBroadcastRequest();

    void framerReseived(QImage frame);
    void sendDatagram();

private:
    //transmitter side
    QTimer *m_cuptutingTimer;
    QTimer *m_transmitingTimer;
    QList<QByteArray> m_datagram;
    int m_frameProcessingTime;
    int m_cuptureProcessingTime;

    //reseiver size
    QByteArray m_frameStorage;
    QByteArray m_checksum;
    bool m_isNewFrame;

    quint16 m_localPort;
    QUdpSocket *m_socket;

    bool m_isCapturing;
    bool m_isReseiving;

    WId m_winId;
    int m_width;
    int m_height;

    QByteArray m_frameFormat;
    int m_framePartSize;

};

#endif // VIDEOUPDSOCKET_H
