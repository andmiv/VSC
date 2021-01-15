#include "ivideoupdsocket.h"

#include <QTimer>
#include <QUdpSocket>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#include <QtConcurrent/QtConcurrent>
#include <QScreen>
#include <QDesktopWidget>

IVideoUpdSocket::IVideoUpdSocket(QObject *parent)
    : QObject(parent)
    , m_isProcessing(false)
    , m_port(0)
    , m_socket(new QUdpSocket(this))
{
}

IVideoUpdSocket::~IVideoUpdSocket()
{

}

quint16 IVideoUpdSocket::port() const
{
    return m_port;
}

void IVideoUpdSocket::setPort(const quint16 &port)
{
    if(isProcessing())
        return;

    if(m_port == port)
        return;

    m_port = port;
}

bool IVideoUpdSocket::isProcessing() const
{
    return m_isProcessing;
}

void IVideoUpdSocket::setIsProcessing(bool processing)
{
    if(m_isProcessing == processing)
        return;
    m_isProcessing = processing;
}

QUdpSocket *IVideoUpdSocket::socket() const
{
    return m_socket;
}
