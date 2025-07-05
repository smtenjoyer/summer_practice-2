#include "myserver.h"

myserver::myserver(){}

myserver::~myserver(){}

void myserver::startServer()
{
    if (this->listen(QHostAddress::Any,5555))
    {
        qDebug()<<"Listening";
    }
    else
    {
        qDebug()<<"Not listening";
    }
}

void myserver::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &myserver::sockReady);
    connect(socket, &QTcpSocket::disconnected, this, &myserver::sockDisc);

    clients.insert(socket);
    qDebug() << socketDescriptor << "Client connected";

    socket->write("You are connected to the server!\n");
}

void myserver::sockReady()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QByteArray data = socket->readAll();
        qDebug() << "Received:" << data;

        socket->write(QString("Server echo: " + data).toUtf8());
    }
}

void myserver::sockDisc()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        qDebug() << socket->socketDescriptor() << "Client disconnected";
        clients.remove(socket);
        socket->deleteLater();
    }
}
