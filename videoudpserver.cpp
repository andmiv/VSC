#include "videoudpserver.h"

#include <QUdpSocket>
#include <QtConcurrent/QtConcurrent>
#include <QImageReader>

VideoUdpServer::VideoUdpServer(QObject *parent)
    : IVideoUpdSocket(parent)
    , m_readyToReceive(true)
    , m_checksum(0)
    , m_datagram()
    , m_futureFrame()
    , m_frameWatcher(new QFutureWatcher<void>(this))
    , m_streamController(Q_NULLPTR)
{
    connect(m_frameWatcher, &QFutureWatcher<void>::finished, this, &VideoUdpServer::frameReseived);
}

VideoUdpServer::~VideoUdpServer()
{
    //TODO на такой вызов виртуального метода в деструкторе общий запрет - переделать
    stopBroadcast();
}

void VideoUdpServer::acceptBroadcast()
{
    setIsProcessing(true);
    connect(socket(), &QUdpSocket::readyRead, this, &VideoUdpServer::receive);
    socket()->bind(QHostAddress::AnyIPv4, port());
}

void VideoUdpServer::stopBroadcast()
{
    disconnect(socket(), &QUdpSocket::readyRead, this, &VideoUdpServer::receive);
    socket()->close();
    m_checksum = 0;
    m_datagram.clear();
    m_readyToReceive = false;
    setIsProcessing(false);
}

VideoStreamController *VideoUdpServer::streamContoroller() const
{
    return m_streamController;
}

void VideoUdpServer::setStreamController(VideoStreamController *controller)
{
    m_streamController = controller;
}

void VideoUdpServer::receive()
{
    QByteArray frame;
    frame.resize(static_cast<int>(socket()->pendingDatagramSize()));
    QHostAddress address(QHostAddress::AnyIPv4);
    quint16 listenedPort = port();
    socket()->readDatagram(frame.data(), frame.size(), &address, &listenedPort);
    QDataStream stream(&frame, QIODevice::ReadOnly);

    if(m_readyToReceive) {
        m_datagram.clear();
        quint16 titleSize = 0;
        stream >> titleSize;
        if(frame.size() != titleSize) {
            qDebug() << "VideoUdpServer::reseive Drop on start";
            return;
        }
        quint16 sourceWidth;
        quint16 sourceHeight;
        QByteArray format;
        stream >> m_checksum >> sourceWidth >> sourceHeight;
        frame.remove(0, sizeof(titleSize) + sizeof(m_checksum) + sizeof(sourceWidth) + sizeof(sourceHeight));
        format = frame;

        VideoStreamSettings settings(format, sourceWidth, sourceHeight);
        m_streamController->setSettings(settings);
        m_readyToReceive = false;
        return;
    }

    if(frame == "end") {
        m_readyToReceive = true;
        if(qChecksum(m_datagram.data(), m_datagram.dataSize()) != m_checksum) {
            qDebug() << "VideoUdpServer::reseive Drop on checksum";
            return;
        }
        //TODO правильно реализовать создание кадра в отдельном потоке
        m_futureFrame = QtConcurrent::run(m_streamController, &VideoStreamController::DatagramToFrame, m_datagram);
        m_frameWatcher->setFuture(m_futureFrame);
        return;
    }
    m_datagram.append(frame);
}
