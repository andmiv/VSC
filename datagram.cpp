#include "datagram.h"

Datagram::Datagram()
    : m_title()
    , m_data()
{

}

Datagram::Datagram(const QByteArray &title,
                   const QByteArray data)
    : m_title(title)
    , m_data(data)
{

}

QByteArray Datagram::getAndEraseFirsBytes(quint16 size)
{
    QByteArray bytes = m_data.left(size);
    m_data.remove(0, size);
    return bytes;
}

bool Datagram::isEmpty() const
{
    return m_title.isEmpty() && m_data.isEmpty();
}

void Datagram::clear()
{
    m_title.clear();
    m_data.clear();
}

void Datagram::append(const QByteArray &bytes)
{
    m_data.append(bytes);
}

int Datagram::titleSize() const
{
    return m_title.size();
}

int Datagram::dataSize() const
{
    return m_data.size();
}

QByteArray Datagram::eraseTitle()
{
    QByteArray title = m_title;
    m_title.clear();
    return title;
}

QByteArray Datagram::data() const
{
    return m_data;
}
