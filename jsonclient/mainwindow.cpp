#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::sockConnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::sockReady);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::sockDisc);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::sockError);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    socket->connectToHost("127.0.0.1",5432);
}

void MainWindow::sockConnected() {
    qDebug() << "Успешно подключено к серверу!";
}

void MainWindow::sockDisc()
{
    qDebug() << "Отключено от сервера!";
    socket->deleteLater();
    socket = nullptr;
}

void MainWindow::sockReady()
{
    if (socket->waitForConnected(500))
    {
        socket->waitForReadyRead(500);
        Data = socket->readAll();
        qDebug()<<Data;
    }
}

    void MainWindow::sockError(QAbstractSocket::SocketError error) {
    qDebug() << "Ошибка сокета: " << socket->errorString();
    QMessageBox::critical(this, "Ошибка подключения", "Не удалось подключиться к серверу: " + socket->errorString());
}
