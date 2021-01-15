#include "reseiver.h"

#include <QPainter>
#include <QImageReader>
#include <QBuffer>
#include <QQuickWindow>
#include <QtConcurrent/QtConcurrent>

Reseiver::Reseiver(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_mainFrame()
    , m_isReseiving(false)
{
    connect(this, &Reseiver::needUpdate, this, &QQuickItem::update);
}

void Reseiver::paint(QPainter *painter)
{
    painter->drawImage(this->boundingRect(), m_mainFrame);
}

void Reseiver::startReseive(QByteArray frame)
{
    if(m_isReseiving)
        return;

    QtConcurrent::run(this, &Reseiver::reseive, frame);
}

void Reseiver::reseive(QByteArray frame)
{
    m_isReseiving = true;
    QBuffer frameBuf(&frame);
    frameBuf.open(QIODevice::ReadOnly);
    QImageReader reader(&frameBuf, "jpg");
    m_mainFrame = reader.read();
    emit needUpdate();
    m_isReseiving = false;
}


