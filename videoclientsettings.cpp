#include "videoclientsettings.h"

VideoClientSettings::VideoClientSettings()
    : m_transmitProcessingTime(0)
    , m_packageCount(0)
    , m_packageSize(0)
{

}

VideoClientSettings::VideoClientSettings(const quint16 &transmitProcessingTime,
                                         const quint16 &packageCount,
                                         const quint16 &packageSize)
    : m_transmitProcessingTime(transmitProcessingTime)
    , m_packageCount(packageCount)
    , m_packageSize(packageSize)
{

}

quint16 VideoClientSettings::tramsmitProcessingTime() const
{
    return m_transmitProcessingTime;
}

quint16 VideoClientSettings::packageCount() const
{
    return m_packageCount;
}

quint16 VideoClientSettings::packageSize() const
{
    return m_packageSize;
}
