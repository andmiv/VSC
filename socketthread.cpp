#include "socketthread.h"

SocketThread::SocketThread(QObject *parent)
    : QThread(parent)
{

}

void SocketThread::run()
{
    QThread::run();
    exec();
}
