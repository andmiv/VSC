#include "videostreamer.h"
#include "socketthread.h"
#include "servicetcpclient.h"
#include "servicetcpserver.h"
#include "videoudpclient.h"
#include "videoudpserver.h"

#include <QTimer>
#include <QPainter>

VideoStreamer::VideoStreamer(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_lastFrame()
    , m_serviceThread(new SocketThread)
    , m_videoThread(new SocketThread)
    , m_serviceClient(new ServiceTcpClient)
    , m_serviceServer(new ServiceTcpServer)
    , m_videoClient(new VideoUdpClient)
    , m_videoServer(new VideoUdpServer)
    , m_tcpAddress(QHostAddress::LocalHost)
    , m_udpAddress(QHostAddress::LocalHost)
    , m_status(NotConnected)
    , m_acceptTimer(new QTimer(this))
    , m_port(0)
    , m_frameFormat()
    , m_acceptTime(5)
{
    //Общая инициализация
    setPort(50000);
    setFrameFormat("png");
    qRegisterMetaType<ConnectionStatus>("ConnectionStatus");
    connect(m_acceptTimer, &QTimer::timeout, [&](){ setAcceptTime(acceptTime() - 1);});

    // Инициализация tcp клиента
    connect(this, &VideoStreamer::sendMessage, m_serviceClient, &ServiceTcpClient::sendMessageToHost);
    connect(m_serviceClient, &ServiceTcpClient::connectionFaild, [=](){
        this->setStatus(ConnectionStatus::ConnectionFaild);
    });

    //Инициализация tcp сервера
    connect(m_serviceServer, &ServiceTcpServer::messageReceived, this, &VideoStreamer::onMessageReceived);

    //Общая инициализация для сервера и клиента
    VideoStreamController *streamController = new VideoStreamController(m_videoClient);
//    connect(streamController, &VideoStreamController::fpsChanged, this, &VideoStreamer::onFpsChanged);
    VideoStreamSettings streamSettings("png", this->width(), this->height(), this->x(), this->y(), 33);
    streamController->setSettings(streamSettings);

    //Инициализация udp клиента
    VideoClientSettings clientSettings(1, 10, 1024);
    m_videoClient->setSettings(clientSettings);
    //!TODO реализовать streamController как sharedPtr
    m_videoClient->setStreamController(streamController);
    connect(this, &VideoStreamer::endTransmitting, m_videoClient, &VideoUdpClient::stopBroadcast);
    connect(this, &VideoStreamer::beginTransmitting, m_videoClient, &VideoUdpClient::startBroadcast);
    connect(this, &QQuickItem::widthChanged, [&](){
        m_videoClient->streamController()->settings().setWidth(this->parentItem()->width());
    });
    connect(this, &QQuickItem::heightChanged, [&](){
        m_videoClient->streamController()->settings().setHeight(this->parentItem()->height());
    });
    connect(this, &QQuickItem::xChanged, [&](){
        m_videoClient->streamController()->settings().setX(this->parentItem()->x());
    });
    connect(this, &QQuickItem::yChanged, [&](){
        m_videoClient->streamController()->settings().setY(this->parentItem()->y());
    });
    connect(m_videoClient, &VideoUdpClient::connectionEstablished, [&](){
        this->setStatus(ConnectionStatus::Transmitting);
    });

    //Для тестов
    VideoStreamController *streamController_1 = new VideoStreamController(m_videoServer);
    connect(streamController_1, &VideoStreamController::fpsChanged, this, &VideoStreamer::onFpsChanged);
    VideoStreamSettings streamSettings_1("png", this->width(), this->height(), this->x(), this->y(), 18);
    streamController_1->setSettings(streamSettings_1);
    //Инициализация udp сервера
    m_videoServer->setStreamController(streamController_1);
    connect(this, &VideoStreamer::acceptStream, m_videoServer, &VideoUdpServer::acceptBroadcast);
    connect(m_videoServer, &VideoUdpServer::frameReseived, this, &VideoStreamer::onFrameReceived);
    connect(this, &VideoStreamer::endReseiving, m_videoServer, &VideoUdpServer::stopBroadcast);
//    connect(m_videoServer, &VideoUdpServer::sizeChanged, [=](quint16 width, quint16 height){
//        this->setWidth(width);
//        this->setHeight(height);
//    });

    m_videoClient->moveToThread(m_videoThread);
    m_videoServer->moveToThread(m_videoThread);
    m_serviceClient->moveToThread(m_serviceThread);
    m_serviceServer->moveToThread(m_serviceThread);

    m_videoThread->start();
    m_serviceThread->start();
}

VideoStreamer::~VideoStreamer()
{
    m_videoThread->exit();
    m_videoThread->quit();
    m_videoThread->deleteLater();
    m_serviceThread->exit();
    m_serviceThread->quit();
    m_videoThread->deleteLater();
}

void VideoStreamer::setFrameFormat(QByteArray frameFormat)
{
    if(m_videoClient->isProcessing()
            || m_videoServer->isProcessing()
            || status() == ConnectionStatus::IncomingReseiveRequest
            || status() == ConnectionStatus::IncomingTransmitRequest
            || status() == ConnectionStatus::OutgoingConnection)
        return;

    if (m_frameFormat == frameFormat)
        return;

    m_frameFormat = frameFormat;

    emit frameFormatChanged(m_frameFormat);
}

void VideoStreamer::transmittingRequest(QString addr)
{
    if(m_videoClient->isProcessing()
            || m_videoServer->isProcessing()
            || status() == ConnectionStatus::IncomingReseiveRequest
            || status() == ConnectionStatus::IncomingTransmitRequest
            || status() == ConnectionStatus::OutgoingConnection) {
        qDebug() << "VideoStreamer::transmittingRequest udp socket already in use";
        return;
    }

    setStatus(OutgoingConnection);
    m_udpAddress = addr;
    m_tcpAddress = addr;
    emit sendMessage(QByteArray::number(static_cast<int>(TransmittingRequest)), m_tcpAddress.toString());
}

void VideoStreamer::receivingRequest(QString addr)
{
    if(m_videoClient->isProcessing()
            || m_videoServer->isProcessing()
            || status() == ConnectionStatus::IncomingReseiveRequest
            || status() == ConnectionStatus::IncomingTransmitRequest
            || status() == ConnectionStatus::OutgoingConnection) {
        qDebug() << "VideoStreamer::transmittingRequest udp socket already in use";
        return;
    }

    setStatus(OutgoingConnection);
    m_udpAddress = addr;
    m_tcpAddress = addr;
    emit sendMessage(QByteArray::number(static_cast<int>(ReseivingRequest)), m_tcpAddress.toString());
}

void VideoStreamer::acceptTransmitRequest()
{
    if(status() != IncomingTransmitRequest) {
        qWarning() << "VideoStreamer::acceptTransmitRequest the incoming request has not been received";
        return;
    }
    if(m_acceptTimer->isActive())
        m_acceptTimer->stop();
    setStatus(Receiving);
    emit acceptStream();
    emit sendMessage(QByteArray::number(static_cast<int>(TransmitRequestAccepted)), m_tcpAddress.toString());
    m_tcpAddress = QHostAddress::LocalHost;
}

void VideoStreamer::denyTransmitRequest()
{
    if(status() != IncomingTransmitRequest) {
        qWarning() << "VideoStreamer::denyTransmitRequest the incoming request has not been received";
        return;
    }
    if(m_acceptTimer->isActive())
        m_acceptTimer->stop();
    emit sendMessage(QByteArray::number(static_cast<int>(TransmitRequestDeny)), m_tcpAddress.toString());
    m_tcpAddress = QHostAddress::LocalHost;
    m_udpAddress = QHostAddress::LocalHost;
}

void VideoStreamer::acceptReceiveRequest()
{
    if(status() != IncomingReseiveRequest) {
        qWarning() << "VideoStreamer::acceptReceiveRequest the incoming request has not been received";
        return;
    }
    if(m_acceptTimer->isActive())
        m_acceptTimer->stop();
    setStatus(Transmitting);
    emit beginTransmitting(m_udpAddress.toString());
    emit sendMessage(QByteArray::number(static_cast<int>(ReceiveRequestAccepted)), m_tcpAddress.toString());
    m_tcpAddress = QHostAddress::LocalHost;
}

void VideoStreamer::denyReceiveRequest()
{
    if(status() != IncomingReseiveRequest) {
        qWarning() << "VideoStreamer::denyTransmitRequest the incoming request has not been received";
        return;
    }
    if(m_acceptTimer->isActive())
        m_acceptTimer->stop();
    emit sendMessage(QByteArray::number(static_cast<int>(ReceiveRequestDeny)), m_tcpAddress.toString());
    m_tcpAddress = QHostAddress::LocalHost;
    m_udpAddress = QHostAddress::LocalHost;
}

void VideoStreamer::stopTransmitting()
{
    emit endTransmitting();
    emit sendMessage(QByteArray::number(static_cast<int>(TransmittingStopped)), m_udpAddress.toString());
    m_udpAddress = QHostAddress::LocalHost;
    setStatus(NotConnected);
    m_lastFrame = QImage();
    update();
}

void VideoStreamer::stopReceiving()
{
    emit endReseiving();
    emit sendMessage(QByteArray::number(static_cast<int>(ReceivingStopped)), m_udpAddress.toString());
    m_udpAddress = QHostAddress::LocalHost;
    setStatus(NotConnected);
    m_lastFrame = QImage();
    update();
}

void VideoStreamer::setAcceptTime(quint16 acceptTime)
{
    if (m_acceptTime == acceptTime)
        return;

    if(m_acceptTime == 1) {
        m_acceptTime = 5;
        m_acceptTimer->stop();
        //Эммитируются сигналы отправляются сообщения о готовности принять и начать втрим,
        //объект класса сам разберется что нужно делать
        acceptReceiveRequest();
        acceptTransmitRequest();
    }
    else
        m_acceptTime = acceptTime;
    emit acceptTimeChanged(m_acceptTime);
}

void VideoStreamer::onMessageReceived(QByteArray msg, QString addr)
{
    ServiceMessage message = ServiceMessage(msg.toInt());
    qDebug() << ServiceMessage(message) << addr;
    m_tcpAddress = addr;
    switch (message) {
    case TransmittingRequest:
        if(m_videoClient->isProcessing() || m_videoServer->isProcessing()){
            qDebug() << "VideoStreamer::onMessageReseived TransmittingRequest: udp socket already in use";
            denyTransmitRequest();
            return;
        }
        m_udpAddress = addr;
        setStatus(IncomingTransmitRequest);
        m_acceptTimer->start(1000);
        break;
    case TransmitRequestAccepted:
                if(status() != OutgoingConnection) {
                    qDebug() << "VideoStreamer::onMessageReseived TransmitRequestAccepted: there was no outgoing request to host" << addr;
                    stopTransmitting();
                    m_tcpAddress = QHostAddress::LocalHost;
                    return;
                }
        if(m_udpAddress.toString() != addr) {
            qDebug() << "VideoStreamer::onMessageReseived TransmitRequestAccepted: the request was answered by an unknown address" << addr;
            stopTransmitting();
            m_tcpAddress = QHostAddress::LocalHost;
            return;
        }
        setStatus(Transmitting);
        emit beginTransmitting(m_udpAddress.toString());
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    case ReseivingRequest:
        if(m_videoClient->isProcessing() || m_videoServer->isProcessing()) {
            qDebug() << "VideoStreamer::onMessageReseived ReseivingRequest: udp socket already in use";
            denyReceiveRequest();
            return;
        }
        m_udpAddress = addr;
        setStatus(IncomingReseiveRequest);
        m_acceptTimer->start(1000);
        break;
    case ReceiveRequestAccepted:
                if(status() != OutgoingConnection) {
                    qDebug() << "VideoStreamer::onMessageReseived ReceiveRequestAccepted: there was no outgoing request to host" << addr;
                    stopReceiving();
                    m_tcpAddress = QHostAddress::LocalHost;
                    return;
                }
        if(m_udpAddress.toString() != addr) {
            qDebug() << "VideoStreamer::onMessageReseived ReceiveRequestAccepted: the request was answered by an unknown address" << addr;
            stopReceiving();
            m_tcpAddress = QHostAddress::LocalHost;
            return;
        }
        setStatus(Receiving);
        emit acceptStream();
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    case ReceiveRequestDeny:
        setStatus(NotConnected);
        m_udpAddress = QHostAddress::LocalHost;
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    case TransmitRequestDeny:
        setStatus(NotConnected);
        m_udpAddress = QHostAddress::LocalHost;
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    case ReceivingStopped:
        emit endTransmitting();
        m_lastFrame = QImage();
        update();
        setStatus(NotConnected);
        m_udpAddress = QHostAddress::LocalHost;
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    case TransmittingStopped:
        emit endReseiving();
        m_lastFrame = QImage();
        update();
        setStatus(NotConnected);
        m_udpAddress = QHostAddress::LocalHost;
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    default:
        qDebug() << "VideoStreamer::onMessageReseived ReceiveRequestAccepted: unknown message reseived from host" << addr;
        m_tcpAddress = QHostAddress::LocalHost;
        break;
    }

}

void VideoStreamer::onFrameReceived()
{
    this->setWidth(m_videoServer->streamContoroller()->settings().width());
    this->setHeight(m_videoServer->streamContoroller()->settings().height());
    update();
}

void VideoStreamer::onFpsChanged(int fps)
{
    if(m_fps == fps)
        return;

    m_fps = fps;
    emit fpsChanged(m_fps);
}

void VideoStreamer::setPort(quint16 port)
{
    if(m_videoClient->isProcessing() || m_videoServer->isProcessing())
        return;
    if(status() == ConnectionStatus::IncomingReseiveRequest
            || status() == ConnectionStatus::IncomingTransmitRequest
            || status() == ConnectionStatus::OutgoingConnection)
        return;
    if (m_port == port)
        return;

    m_port = port;
    m_serviceClient->setPort(m_port);
    m_serviceServer->setPort(m_port);
    m_videoClient->setPort(m_port);
    m_videoServer->setPort(m_port);

    emit portChanged(m_port);
}

void VideoStreamer::setStatus(VideoStreamer::ConnectionStatus status)
{
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged(m_status);
}

VideoStreamer::ConnectionStatus VideoStreamer::status() const
{
    return m_status;
}

quint16 VideoStreamer::port() const
{
    return m_port;
}

QByteArray VideoStreamer::frameFormat() const
{
    return m_frameFormat;
}

QImage VideoStreamer::lastFrame() const
{
    return m_lastFrame;
}

void VideoStreamer::paint(QPainter *painter)
{
    painter->drawImage(this->boundingRect(), m_videoServer->streamContoroller()->frame());
}

int VideoStreamer::fps() const
{
    return m_fps;
}

quint16 VideoStreamer::acceptTime() const
{
    return m_acceptTime;
}
