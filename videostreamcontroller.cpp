#include "videostreamcontroller.h"
#include "datagram.h"

#include <QTimer>
#include <QImage>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QBuffer>
#include <QImageWriter>
#include <QImageReader>

#include <QDebug>

VideoStreamController::VideoStreamController(QObject *parent)
    : QObject(parent)
    , m_cuptureTimer(new QTimer(this))
    , m_fpsTimer(new QTimer(this))
    , m_settings()
    , m_fps(0)
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
    QByteArray data;
    {
        QBuffer frameBuf(&data);
        frameBuf.open(QBuffer::WriteOnly);
        QImageWriter writer(&frameBuf, m_settings.format());
        writer.write(m_mainFrame);
        frameBuf.close();
    }
    QByteArray title;
    {
        QDataStream stream(&title, QIODevice::WriteOnly);
        quint16 checksum = qChecksum(data.data(), data.size());
        quint16 titleSize = (8 + m_settings.format().size());
        stream << titleSize << checksum << m_settings.width() << m_settings.height();
        title.append(m_settings.format());
    }
    return Datagram(title, data);
}

void VideoStreamController::DatagramToFrame(Datagram datagram)
{
    QByteArray frame = datagram.data();
    QBuffer frameBuf(&frame);
    if(!frameBuf.open(QIODevice::ReadOnly)) {
        qDebug() << "VideoStreamController::DatagramToFrame invalid frame";
        return;
    }
    QImageReader reader(&frameBuf, m_settings.format());
    if(!reader.canRead()) {
        qDebug() << "VideoStreamController::DatagramToFrame cannot read frame";
        return;
    }
    m_mainFrame = reader.read();
    if(reader.error()) {
        return;
    }
    ++m_fps;
}

QImage VideoStreamController::frame() const
{
    return m_mainFrame;
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
    m_mainFrame = QApplication::screens().at(0)->grabWindow(QApplication::desktop()->winId(),
                                                            m_settings.x(), m_settings.y(),
                                                            m_settings.width(), m_settings.height()).toImage();
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

