#ifndef VIDEOSTREAMSETTINGS_H
#define VIDEOSTREAMSETTINGS_H

#include <QtGlobal>
#include <QByteArray>

class VideoStreamSettings
{
public:
    VideoStreamSettings();
    VideoStreamSettings(const QByteArray &format,
                        const quint16 &width, const quint16 &height,
                        const quint16 &x, const quint16 &y,
                        const quint16 &fps);
    VideoStreamSettings(const QByteArray &format,
                        const quint16 &width, const quint16 &height);
    VideoStreamSettings(const VideoStreamSettings &other);
    VideoStreamSettings &operator=(const VideoStreamSettings &other);


    quint16 x() const;
    quint16 y() const;
    quint16 width() const;
    quint16 height() const;
    QByteArray format() const;
    quint16 fps() const;

    void setX(const quint16 &x);
    void setY(const quint16 &y);
    void setWidth(const quint16 &width);
    void setHeight(const quint16 &height);
    void setFormat(const QByteArray &format);
    void setFps(const quint16 &fps);

private:
    QByteArray m_format;
    quint16 m_x;
    quint16 m_y;
    quint16 m_width;
    quint16 m_height;
    quint16 m_fps;
};

#endif // VIDEOSTREAMSETTINGS_H
