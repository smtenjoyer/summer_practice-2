#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextBrowser>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::sockReady);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::sockDisc);

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendMessage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString ip = ui->ipLineEdit->text();
    socket->connectToHost(ip, 5555);

    if(socket->waitForConnected(1000)) {
        ui->statusLabel->setText("Connected to server");
    } else {
        ui->statusLabel->setText("Connection failed");
    }
}

void MainWindow::sendMessage()
{
    QString message = ui->messageLineEdit->text();
    socket->write(message.toUtf8());
    ui->messageLineEdit->clear();
}

void MainWindow::sockReady()
{
    QByteArray data = socket->readAll();
    ui->textBrowser->append("Server: " + QString(data));
}

void MainWindow::sockDisc() {
    if (socket) {
        ui->statusLabel->setText("Disconnected");
        socket->deleteLater();
        socket = nullptr;
    }
}
