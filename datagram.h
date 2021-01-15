#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <QtGlobal>
#include <QByteArray>

class Datagram
{
public:
    Datagram();
    Datagram(const QByteArray &title,
             const QByteArray data);

    int titleSize() const;
    int dataSize() const;

    QByteArray eraseTitle();
    QByteArray data() const;
    QByteArray getAndEraseFirsBytes(quint16 size);
    bool isEmpty() const;
    void clear();
    void append(const QByteArray &bytes);

private:
    QByteArray m_title;
    QByteArray m_data;
};

#endif // DATAGRAM_H
