#ifndef RESEIVER_H
#define RESEIVER_H

#include <QQuickPaintedItem>
#include <QImage>

class Reseiver : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit Reseiver(QQuickItem *parent = nullptr);
    virtual void paint(QPainter *painter) override;

    void startReseive(QByteArray frame);

public slots:
    void reseive(QByteArray frame);

signals:
    void needUpdate();

private:
    QImage m_mainFrame;
    bool m_isReseiving;
};

#endif // RESEIVER_H
