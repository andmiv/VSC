#include "videostreamsettings.h"

VideoStreamSettings::VideoStreamSettings()
    : m_format("jpg")
    , m_x(0)
    , m_y(0)
    , m_width(0)
    , m_height(1)
    , m_fps(0)
{

}

VideoStreamSettings::VideoStreamSettings(const QByteArray &format,
                                         const quint16 &width, const quint16 &height,
                                         const quint16 &x, const quint16 &y,
                                         const quint16 &fps)
    : m_format(format)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_fps(fps)
{

}

VideoStreamSettings::VideoStreamSettings(const QByteArray &format,
                                         const quint16 &width,
                                         const quint16 &height)
    : m_format(format)
    , m_x(0)
    , m_y(0)
    , m_width(width)
    , m_height(height)
    , m_fps(0)
{

}

VideoStreamSettings::VideoStreamSettings(const VideoStreamSettings &other)
    : m_format(other.m_format)
    , m_x(other.m_x)
    , m_y(other.m_y)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_fps(other.m_fps)
{

}

VideoStreamSettings &VideoStreamSettings::operator =(const VideoStreamSettings &other)
{
    if(this == &other)
        return *this;

    this->m_format = other.m_format;
    this->m_x = other.m_x;
    this->m_y = other.m_y;
    this->m_width = other.m_width;
    this->m_height = other.m_height;
    this->m_fps = other.m_fps;
    return *this;
}

quint16 VideoStreamSettings::x() const
{
    return m_x;
}

quint16 VideoStreamSettings::y() const
{
    return m_y;
}

quint16 VideoStreamSettings::width() const
{
    return m_width;
}

quint16 VideoStreamSettings::height() const
{
    return m_height;
}

QByteArray VideoStreamSettings::format() const
{
    return m_format;
}

quint16 VideoStreamSettings::fps() const
{
    return m_fps;
}

void VideoStreamSettings::setX(const quint16 &x)
{
    if(m_x == x)
        return;

    m_x = x;
}

void VideoStreamSettings::setY(const quint16 &y)
{
    if(m_y == y)
        return;

    m_y = y;
}

void VideoStreamSettings::setWidth(const quint16 &width)
{
    if(m_width == width)
        return;

    m_width = width;
}

void VideoStreamSettings::setHeight(const quint16 &height)
{
    if(m_height == height)
        return;

    m_height = height;
}

void VideoStreamSettings::setFormat(const QByteArray &format)
{
    if(m_format == format)
        return;

    m_format = format;
}

void VideoStreamSettings::setFps(const quint16 &fps)
{
    if(m_fps == fps)
        return;

    m_fps = fps;
}
