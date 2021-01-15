#ifndef VIDEOSTREAMCONTROLLER_H
#define VIDEOSTREAMCONTROLLER_H

#include <QObject>
#include <QImage>

#include "videostreamsettings.h"

class QTimer;
class Datagram;

class VideoStreamController : public QObject
{
    Q_OBJECT
public:
    explicit VideoStreamController(QObject *parent = nullptr);

    void startFrameCupture();
    void stopFrameCupture();

    Datagram frameToDatagram();
    void DatagramToFrame(Datagram datagram);

    VideoStreamSettings &settings();
    void setSettings(const VideoStreamSettings &settings);

    QImage frame() const;

private slots:
    void cupture();
    void resetTimer();

signals:
    void frameCuptured();

private:
    QTimer *m_cuptureTimer;
    VideoStreamSettings m_settings;
    QImage m_mainFrame;
};

#endif // VIDEOSTREAMCONTROLLER_H
