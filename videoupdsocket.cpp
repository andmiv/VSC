#include "videoupdsocket.h"

#include <QTimer>
#include <QUdpSocket>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#include <QtConcurrent/QtConcurrent>
#include <QScreen>
#include <QDesktopWidget>

VideoUpdSocket::VideoUpdSocket(const quint16 &localPort,
                               const QByteArray &frameFormat,
                               QObject *parent)
    : QObject(parent)
    , m_cuptutingTimer(new QTimer)
    , m_transmitingTimer(new QTimer)
    , m_datagram()
    , m_frameProcessingTime(10)
    , m_cuptureProcessingTime(1)
    , m_frameStorage()
    , m_checksum()
    , m_isNewFrame(false)
    , m_localPort(localPort)
    , m_socket(new QUdpSocket)
    , m_isCapturing(false)
    , m_isReseiving(false)
    , m_winId()
    , m_width(0)
    , m_height(1)
    , m_frameFormat(frameFormat)
    , m_framePartSize(1024)
{
    connect(this, &VideoUpdSocket::widthChanged, this, &VideoUpdSocket::setWidth);
    connect(this, &VideoUpdSocket::heightChanged, this, &VideoUpdSocket::setHeight);
    connect(this, &VideoUpdSocket::framePartSizeChenged, this, &VideoUpdSocket::setFramePartSize);
//    connect(this, &VideoUpdSocket::frameFormatChanged, this, &VideoUpdSocket::setFrameFormat);
//    connect(this, &VideoUpdSocket::localPortChanged, this, &VideoUpdSocket::setLocalPort);
}

void VideoUpdSocket::setWidth(int width)
{
    if(m_width == width)
        return;
    m_width = width;
}

void VideoUpdSocket::setHeight(int height)
{
    if(m_height == height)
        return;
    m_height = height;
}

void VideoUpdSocket::setFramePartSize(int size)
{
    if(m_framePartSize == size)
        return;
    m_framePartSize = size;
}

void VideoUpdSocket::startBroadcast(QHostAddress addr, WId winId)
{
    m_winId = winId;

    connect(this, &VideoUpdSocket::sendDatagram, this, &VideoUpdSocket::initFrameTransmit);
    m_socket->connectToHost(addr, m_localPort);
    m_socket->open(QIODevice::WriteOnly);

    connect(m_cuptutingTimer, &QTimer::timeout, this, &VideoUpdSocket::cupture);
    m_cuptutingTimer->start(33);
}

void VideoUpdSocket::asseptBreadcast()
{
    connect(m_socket, &QUdpSocket::readyRead, this, &VideoUpdSocket::reseive);
    disconnect(this, &VideoUpdSocket::sendDatagram, this, &VideoUpdSocket::initFrameTransmit);
    m_socket->bind(QHostAddress::AnyIPv4, m_localPort);
}

void VideoUpdSocket::stopBroadcast()
{
    m_cuptutingTimer->stop();
    m_transmitingTimer->stop();
    m_socket->disconnectFromHost();
    m_socket->close();
    disconnect(m_socket, &QUdpSocket::readyRead, this, &VideoUpdSocket::reseive);
    disconnect(this, &VideoUpdSocket::sendDatagram, this, &VideoUpdSocket::initFrameTransmit);
    m_winId = -1;
}

void VideoUpdSocket::initFrameTransmit()
{
    if(!m_datagram.isEmpty())
        return;

    connect(m_transmitingTimer, &QTimer::timeout, this, &VideoUpdSocket::send);
    m_cuptutingTimer->start(m_frameProcessingTime);
}

void VideoUpdSocket::endFrameTransmit()
{
    m_transmitingTimer->stop();
    disconnect(m_transmitingTimer, &QTimer::timeout, this, &VideoUpdSocket::send);
}

void VideoUpdSocket::send()
{
    QList<QByteArray>::iterator iter = m_datagram.begin();
    for(int i = 0; i < m_frameProcessingTime && !m_datagram.isEmpty(); ++i) {
        short status = m_socket->write(*iter);
        if(status < 0) qDebug() << "VideoUdpSocket::send datagram transmit error:" << m_socket->errorString();
        m_datagram.removeFirst();
        iter = m_datagram.begin();
    }
    if(m_datagram.isEmpty()) endFrameTransmit();
}

void VideoUpdSocket::cupture()
{
    if(m_isCapturing)
        return;

    QtConcurrent::run(this, &VideoUpdSocket::createDatagram, m_winId, m_width, m_height, m_framePartSize);
}

void VideoUpdSocket::createDatagram(WId winId, int width, int height, int framePartSize)
{
    m_isCapturing = true;
    QPixmap captFrame = QApplication::screens().at(0)->grabWindow(winId, 0, 0, width, height);
    QByteArray frameArr;
    QBuffer frameBuf(&frameArr);
    frameBuf.open(QBuffer::WriteOnly);
    QImageWriter writer(&frameBuf, m_frameFormat);
    writer.write(captFrame.toImage());
    QList<QByteArray> frameList;
    QByteArray frame;
    frame = QByteArray::number(qChecksum(frameArr.data(), frameArr.size()));
    frame.prepend("start");
    frameList.append(frame);
    frame.clear();
    while(!frameArr.isEmpty()) {
        frame = frameArr.left(framePartSize);
        frameArr.replace(0, framePartSize, "");
        frameList.append(frame);
        frame.clear();
    }
    frameList.append("end");
    emit send();
    m_isCapturing = false;
}

void VideoUpdSocket::reseive()
{
    QByteArray frame;
    frame.resize(static_cast<int>(m_socket->pendingDatagramSize()));
    QHostAddress address(QHostAddress::AnyIPv4);
    m_socket->readDatagram(frame.data(), frame.size(), &address, &m_localPort);

    if(m_isNewFrame) {
        if(!frame.startsWith("start")) {
//            qDebug() << "Drop on start";
            m_frameStorage.clear();
            return;
        }
        frame.replace(0, 5, "");
        m_checksum = frame;
        m_frameStorage.clear();
        m_isNewFrame = false;
        return;
    }

    if(frame.endsWith("end")) {
        m_isNewFrame = true;
        if(QByteArray::number(qChecksum(m_frameStorage.data(), m_frameStorage.size())) != m_checksum) {
//            qDebug() << "Drop on checksum";
            m_frameStorage.clear();
            return;
        }
        QtConcurrent::run(this, &VideoUpdSocket::createFrame, m_frameStorage);
        m_frameStorage.clear();
        return;
    }
    m_frameStorage.append(frame);
}

void VideoUpdSocket::createFrame(QByteArray frame)
{
    if(m_isReseiving)
        return;

    m_isReseiving = true;
    QBuffer frameBuf(&frame);
    if(!frameBuf.open(QIODevice::ReadOnly)) {
        qDebug() << "VideoUpdSocket::createFrame invalid frame";
        m_isReseiving = false;
        return;
    }
    QImageReader reader(&frameBuf, m_frameFormat);
    if(!reader.canRead()) {
        qDebug() << "VideoUpdSocket::createFrame cannot read frame";
        m_isReseiving = false;
        return;
    }
    QImage mainFrame = reader.read();
    if(reader.error()) {
        m_isReseiving = false;
        return;
    }
    if(mainFrame.isNull()) {
        qDebug() << "VideoUpdSocket::createFrame null frame";
        m_isReseiving = false;
        return;
    }
    m_isReseiving = false;
    emit framerReseived(mainFrame);
}
