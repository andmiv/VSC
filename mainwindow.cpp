#include "mainwindow.h"

MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
    , m_view(nullptr)
    , m_engine(nullptr)
    , m_sender(nullptr)
{
    qmlRegisterType<VideoStreamer>("videostream", 1, 0, "VideoStreamer");

    m_engine = new QQmlEngine;
    m_view = new QQuickView(m_engine, nullptr);
    m_view->setSource(QUrl("qrc:/main.qml"));

    m_sender = m_view->rootObject()->findChild<VideoStreamer *>("sender");
}

void MainWindow::open()
{
    m_view->setResizeMode(QQuickView::ResizeMode::SizeRootObjectToView);

    m_view->setWidth(1960);
    m_view->setHeight(1080);
    m_view->show();
}
