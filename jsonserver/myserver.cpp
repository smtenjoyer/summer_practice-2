#include "myserver.h"
#include <QTimer>

myserver::myserver(){}

myserver::~myserver(){}

void myserver::startServer()
{
    if (this->listen(QHostAddress::Any,5432))
    {
        qDebug()<<"Listening";
    }
    else
    {
        qDebug()<<"Not listening";
    }
}

void myserver::incomingConnection(int socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Ошибка установки дескриптора сокета!";
        socket->deleteLater();
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &myserver::sockReady);
    connect(socket, &QTcpSocket::disconnected, this, &myserver::sockDisc);

    qDebug() << socketDescriptor << " Client connected";

    QTimer::singleShot(100, [socket]() {
        socket->write("You are connected\r\n");
        qDebug() << "Send client connect status - YES";
    });
}

void myserver::sockReady()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());

    if (!socket) {
        qDebug() << "Ошибка: Не удалось получить сокет в sockReady!";
        return;
    }

    QByteArray data = socket->readAll();
    qDebug() << "Получено от клиента: " << data;
}

void myserver::sockDisc()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(socket){
        qDebug() << "Client disconnected";
        socket->deleteLater();
    }
}
