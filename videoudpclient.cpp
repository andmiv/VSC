#include "videoudpclient.h"
#include "videostreamcontroller.h"

#include <QUdpSocket>
#include <QTimer>

VideoUdpClient::VideoUdpClient(QObject *parent)
    : IVideoUpdSocket(parent)
    , m_datagramFuture()
    , m_datagramWatcher(new QFutureWatcher<Datagram>(this))
    , m_datagram()
    , m_transmittingTimer(new QTimer(this))
    , m_streamController(Q_NULLPTR)
    , m_settings()
{
    connect(socket(), &QUdpSocket::connected, this, &VideoUdpClient::connected);
    connect(m_datagramWatcher, &QFutureWatcher<Datagram>::finished, this, &VideoUdpClient::saveDatagram);
}

VideoUdpClient::~VideoUdpClient()
{
    stopBroadcast();
    if(m_datagramWatcher->isRunning())
        m_datagramWatcher->cancel();
    delete m_datagramWatcher;
}

bool VideoUdpClient::startBroadcast(QString addr)
{
    if(isProcessing())
        return false;

    if(!m_streamController)
        qFatal("Unable to start a broadcast. VideoStreamController was not instantiated");

    setIsProcessing(true);

    socket()->connectToHost(addr, port());
    socket()->open(QUdpSocket::WriteOnly);

    m_streamController->startFrameCupture();

    return true;
}

VideoStreamController *VideoUdpClient::streamController()
{
    return m_streamController;
}

void VideoUdpClient::setStreamController(VideoStreamController *controller)
{
    m_streamController = controller;
    connect(m_streamController, &VideoStreamController::frameCuptured, this, &VideoUdpClient::getDatagram);
}

VideoClientSettings &VideoUdpClient::settings()
{
    return m_settings;
}

void VideoUdpClient::setSettings(const VideoClientSettings &settings)
{
    m_settings = settings;
}

void VideoUdpClient::saveDatagram()
{
    if(!m_datagram.isEmpty())
        return;
    m_datagram = m_datagramFuture.result();
    initFrameTransmit();
}

void VideoUdpClient::connected()
{
    emit this->connectionEstablished(this->socket()->peerAddress().toString(), this->socket()->peerPort());
}

void VideoUdpClient::stopBroadcast()
{
    if(!isProcessing())
        return;

    m_streamController->stopFrameCupture();
    endFrameTransmit();
    disconnect(this, &VideoUdpClient::send, this, &VideoUdpClient::initFrameTransmit);
    socket()->disconnectFromHost();
    socket()->close();

    setIsProcessing(false);
}

void VideoUdpClient::initFrameTransmit()
{
    short status = socket()->write(m_datagram.eraseTitle());
    if(status < 0) {
        emit transmitionError(socket()->errorString());
        stopBroadcast();
        return;
    }

    connect(m_transmittingTimer, &QTimer::timeout, this, &VideoUdpClient::sendDatagram);
    m_transmittingTimer->start(m_settings.tramsmitProcessingTime());
}

void VideoUdpClient::endFrameTransmit()
{
    m_transmittingTimer->stop();
    disconnect(m_transmittingTimer, &QTimer::timeout, this, &VideoUdpClient::sendDatagram);
    short status = socket()->write("end");
    if(status < 0) {
        emit transmitionError(socket()->errorString());
        stopBroadcast();
        return;
    }
    m_datagram.clear();
}

void VideoUdpClient::sendDatagram()
{
    for(int i = 0; i < m_settings.packageCount() && !m_datagram.isEmpty(); ++i) {
        short status = socket()->write(m_datagram.getAndEraseFirsBytes(m_settings.packageSize()));
        if(status < 0) {
            emit transmitionError(socket()->errorString());
            stopBroadcast();
            return;
        }
    }
    if(m_datagram.isEmpty()) { endFrameTransmit();}
}

void VideoUdpClient::getDatagram()
{
    if(m_datagramWatcher->isRunning())
        return;
    m_datagramFuture = QtConcurrent::run(m_streamController, &VideoStreamController::frameToDatagram);
    m_datagramWatcher->setFuture(m_datagramFuture);
}
