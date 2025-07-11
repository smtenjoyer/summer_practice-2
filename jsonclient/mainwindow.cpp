#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    // Signals and slots connections in MainWindow

    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MainWindow::onError);
    connect(m_socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_connectButton_clicked(){
    QString name = ui->nameEdit->text().trimmed();
    QString IP = ui->IPLineEdit->text();

    if (name.isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Введите имя игрока");
        return;
    }

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


    m_playerName = name;
    m_socket->connectToHost(IP, port);
}

void MainWindow::onConnected(){
    ui->statusLabel->setText("Подключено к серверу");
    ui->connectButton->setEnabled(false);

    // Create and show the GameWindow
    m_gameWindow = new GameWindow(m_socket, m_playerName, this); // Pass socket and player name
    m_gameWindow->show();

    QJsonObject message;
    message["type"] = "register";
    message["name"] = m_playerName;
    sendJsonMessage(message); // Send message
    connect(m_gameWindow, &GameWindow::sendMessage, this, &MainWindow::sendJsonMessage);

}

void MainWindow::onReadyRead()
{
    static QByteArray buffer;

    buffer += m_socket->readAll();

    while (buffer.contains('\n')) {
        int pos = buffer.indexOf('\n');
        QByteArray messageData = buffer.left(pos);
        buffer.remove(0, pos + 1);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);

        if (error.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << error.errorString();
            continue;
        }

        if (doc.isObject()) {
            if (m_gameWindow) {
                m_gameWindow->processServerMessage(doc.object()); // Pass to game window
            }
        }
    }
}

void MainWindow::sendJsonMessage(const QJsonObject &message){

    if (m_socket->state() == QTcpSocket::ConnectedState){

        QJsonDocument doc(message);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        m_socket->write(jsonData + "\n");
        m_socket->flush();
    }
}

void MainWindow::onDisconnected()
{
    ui->statusLabel->setText("Отключено от сервера");
    ui->connectButton->setEnabled(true);

    if (m_gameWindow) {
        m_gameWindow->close();
        m_gameWindow->deleteLater();
        m_gameWindow = nullptr;
    }
}

void MainWindow::onError(QAbstractSocket::SocketError error){
    QMessageBox::warning(this, "Ошибка подключения", m_socket->errorString());
    onDisconnected();
}
