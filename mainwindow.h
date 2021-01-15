#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QQuickView>
#include <QQmlEngine>

#include "videostreamer.h"

class MainWindow : public QObject
{
    Q_OBJECT
public:
    explicit MainWindow(QObject *parent = nullptr);
    void open();

private:
    QQuickView *m_view;
    QQmlEngine *m_engine;
    VideoStreamer *m_sender;
};

#endif // MAINWINDOW_H
