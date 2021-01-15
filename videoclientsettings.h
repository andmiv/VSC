#ifndef VIDEOCLIENTSETTINGS_H
#define VIDEOCLIENTSETTINGS_H

#include <QtGlobal>

class VideoClientSettings
{
public:
    VideoClientSettings();
    VideoClientSettings(const quint16 &transmitProcessingTime,
                        const quint16 &packageCount,
                        const quint16 &packageSize);

    quint16 tramsmitProcessingTime() const;
    quint16 packageCount() const;
    quint16 packageSize() const;

private:
    //! Задержка между передачей packegeCount пакетов
    quint16 m_transmitProcessingTime;
    //! Количество передаваемых за один проход пакетов
    quint16 m_packageCount;
    //! Размер одного пакета
    quint16 m_packageSize;

};

#endif // VIDEOCLIENTSETTINGS_H
