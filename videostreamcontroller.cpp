#include "videostreamcontroller.h"
#include "datagram.h"

#include <QTimer>
#include <QImage>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QDebug>

VideoStreamController::VideoStreamController(QObject *parent)
    : QObject(parent)
    , m_cuptureTimer(new QTimer(this))
    , m_fpsTimer(new QTimer(this))
    , m_settings()
    , m_containers()
    , m_maxFramesCount(3)
    , m_creating(false)
    , m_fps(0)
    , m_decoding(false)
{
    connect(m_fpsTimer, &QTimer::timeout, this, &VideoStreamController::onFpsNeedUpdate);
    m_fpsTimer->start(1000);
}

void VideoStreamController::startFrameCupture()
{
    if(!m_settings.fps())
        return;

    connect(m_cuptureTimer, &QTimer::timeout, this, &VideoStreamController::cupture);
    m_cuptureTimer->start(m_settings.fps());
}

void VideoStreamController::stopFrameCupture()
{
    disconnect(m_cuptureTimer, &QTimer::timeout, this, &VideoStreamController::cupture);
    m_cuptureTimer->stop();

    disconnect(m_fpsTimer, &QTimer::timeout, this, &VideoStreamController::onFpsNeedUpdate);
    m_fpsTimer->stop();
}

Datagram VideoStreamController::frameToDatagram()
{
    if(m_creating)
        return Datagram();

    m_creating = true;

    FrameContainer *f = m_containers.dequeue();
    if(!f) {
        qDebug() << "sender is empty";
        return Datagram();
    }
    f->codeFrame();
    QByteArray data = f->toByteArray();
    delete f;

    QByteArray title;
    {
        QDataStream stream(&title, QIODevice::WriteOnly);
        quint16 checksum = qChecksum(data.data(), data.size());
        quint16 titleSize = (8 + m_settings.format().size());
        stream << titleSize << checksum << m_settings.width() << m_settings.height();
        title.append(m_settings.format());
    }
    m_creating = false;
    return Datagram(title, data);
}

void VideoStreamController::DatagramToFrame(Datagram datagram)
{
    if(m_decoding)
        return;

    m_decoding = true;

    FrameContainer *cont = FrameContainer::fromByteArray(datagram.data(), m_settings.width(), m_settings.height());
    if(!cont) {
        qDebug() << "receiver is empty";
        return;
    }
    cont->decodeFrame();
    m_sourceFrame = cont->toQImage();

    ++m_fps;
    m_decoding = false;
}

QImage VideoStreamController::frame() const
{
    return m_sourceFrame;
}

VideoStreamSettings &VideoStreamController::settings()
{
    return m_settings;
}

void VideoStreamController::setSettings(const VideoStreamSettings &settings)
{
    m_settings = settings;
    resetTimer();
}

void VideoStreamController::cupture()
{
    if(m_creating || m_maxFramesCount < m_containers.size())
        return;

    m_containers.enqueue(
                FrameContainer::fromImage(
                    QApplication::screens().at(0)->grabWindow(
                        QApplication::desktop()->winId(),
                        m_settings.x(), m_settings.y(),
                        m_settings.width(), m_settings.height()).toImage()));

    emit frameCuptured();
}

void VideoStreamController::resetTimer()
{
    if(!m_cuptureTimer->isActive())
        return;

    m_cuptureTimer->stop();
    m_cuptureTimer->start(m_settings.fps());
}

void VideoStreamController::onFpsNeedUpdate()
{
    int fps = m_fps;
    m_fps = 0;
    emit fpsChanged(fps);
}

