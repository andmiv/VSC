#include "iservicetcpsocket.h"

#include <QTcpServer>
#include <QTcpSocket>

IServiceTcpSocket::IServiceTcpSocket(QObject *parent)
    : QObject(parent)
    , m_port(0)
{

}

IServiceTcpSocket::~IServiceTcpSocket()
{

}

quint16 IServiceTcpSocket::port() const
{
    return m_port;
}

void IServiceTcpSocket::setPort(const quint16 &port)
{
    if(m_port == port)
        return;
    m_port = port;
}
