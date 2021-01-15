#ifndef SOCKETTHREAD_H
#define SOCKETTHREAD_H

#include <QThread>

class SocketThread : public QThread
{
    Q_OBJECT
public:
    SocketThread(QObject *parent = nullptr);

protected:
    void run();
};

#endif // SOCKETTHREAD_H
