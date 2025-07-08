#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);
    connect(socket,SIGNAL(readyRead()),this,SLOT(sockReady()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(sockDisc()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString IP = ui->IPLineEdit->text();

    QRegularExpression ipRegex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    if (!ipRegex.match(IP).hasMatch()) {
        QMessageBox::warning(this, "Ошибка", "Неверный формат IP-адреса.");
        return;
    }

    QHostAddress addr(IP);
    if(addr.protocol() == QAbstractSocket::UnknownSocketType){
        QMessageBox::warning(this, "Ошибка", "Неверный IP-адрес (ошибка QHostAddress).");
        return;
    }

    bool ok;
    int port = ui->portLineEdit->text().toInt(&ok);

    if (!ok) {
        QMessageBox::warning(this, "Ошибка", "Неверный формат порта.  Введите число.");
        return;
    }

    if ((port < 1) || (port > 65535)) {
        QMessageBox::warning(this, "Ошибка", "Порт должен быть в диапазоне от 1 до 65535.");
        return;
    }

    socket->connectToHost(IP, port);
    qDebug() << "Попытка подключения к " << IP << ": " << port;

}

void MainWindow::sockDisc()
{
    socket->deleteLater();
}

void MainWindow::sockReady()
{
    if (socket->waitForConnected(500))
    {
        socket->waitForReadyRead(500);
        Data = socket->readAll();

        doc = QJsonDocument::fromJson(Data, &docError);
        if (docError.errorString().toInt()==QJsonParseError::NoError)
        {
            if ((doc.object().value("type").toString() == "connection" ) && (doc.object().value("status").toString() == "yes" ))
            {
                QMessageBox::information(this, "Информация", "Соединение установлено");
            } else {
                QMessageBox::information(this, "Информация", "Соединение не установлено");
            }
        } else {
            QMessageBox::information(this, "Информация", "Ошибки с форматом передачи данных" + docError.errorString());
        }

        qDebug()<<Data;
    }
}
