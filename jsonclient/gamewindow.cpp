#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "DoodleArea.h"
#include <QJsonDocument>
#include <QDataStream>
#include <QDebug>

GameWindow::GameWindow(QTcpSocket* socket, const QString& playerName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GameWindow),
    m_socket(socket),
    m_playerName(playerName),
    m_isDrawing(false),
    m_doodleArea(nullptr)
{
    ui->setupUi(this);

    m_socket->setParent(this);

    connect(ui->sendGuessButton, &QPushButton::clicked, this, &GameWindow::on_sendGuessButton_clicked);

    // Подключаем сигнал readyRead для чтения сообщений с сервера
    connect(m_socket, &QTcpSocket::readyRead, this, &GameWindow::readServerMessages);

    setupGameUI(false);

    QSize newSize(800, 600);
    m_doodleArea = new DoodleArea(newSize);
    setCentralWidget(m_doodleArea);

    // Подключаем сигнал изменения точек рисования к отправке на сервер
    connect(m_doodleArea, &DoodleArea::drawingPointsChanged,
            this, &GameWindow::sendDrawingPoints);
}

GameWindow::~GameWindow()
{
    delete ui;
}

void GameWindow::readServerMessages()
{
    static quint32 blockSize = 0;
    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    while (true) {
        if (blockSize == 0) {
            if (m_socket->bytesAvailable() < (int)sizeof(quint32))
                return;
            in >> blockSize;
        }
        if (m_socket->bytesAvailable() < blockSize)
            return;

        QByteArray jsonData;
        jsonData.resize(blockSize);
        in.readRawData(jsonData.data(), blockSize);

        blockSize = 0;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << parseError.errorString();
            return;
        }

        processServerMessage(doc.object());
    }
}

void GameWindow::processServerMessage(const QJsonObject &message)
{
    QString type = message["type"].toString();

    if (type == "roundStart") {
        QString drawer = message["drawer"].toString();
        bool isDrawer = (drawer == m_playerName);

        m_isDrawing = isDrawer;
        setupGameUI(isDrawer);

        if (isDrawer) {
            QString word = message["word"].toString();
            ui->wordLabel->setText("Нарисуй-ка мне " + word);
        } else {
            ui->wordLabel->setText("Что рисует " + drawer + "?");
        }
    } else if (type == "draw") {
        if (!m_isDrawing) {
            QJsonArray pointsArray = message["points"].toArray();
            QVector<QPoint> points;

            for (const QJsonValue& val : pointsArray) {
                QJsonObject pointObj = val.toObject();
                points.append(QPoint(pointObj["x"].toInt(), pointObj["y"].toInt()));
            }

            // Передать точки для отрисовки на canvas
            if (m_doodleArea) {
                m_doodleArea->drawRemotePoints(points);
            }
        }
    }
    else if (type == "chat") {
        QString player = message["player"].toString();
        QString text = message["text"].toString();
        ui->chatText->append(player + ": " + text);
    }
    else if (type == "correctGuess") {
        QString guesser = message["guesser"].toString();
        QString word = message["word"].toString();
        ui->chatText->append("*** " + guesser + " угадал слово: " + word + " ***");

        // Обновление таблицы очков
        QJsonObject scores = message["scores"].toObject();
        // TODO: заполнение таблицы очков
    }
}

void GameWindow::setupGameUI(bool isDrawer)
{
    if (isDrawer){
        ui->wordLabel->setText("Ваш ход рисовать!");
        ui->sendGuessButton->setEnabled(false);
        ui->guessEdit->setEnabled(false);
    } else {
        ui->wordLabel->setText("Угадывайте что рисуют!");
        ui->sendGuessButton->setEnabled(true);
        ui->guessEdit->setEnabled(true);
    }
}

void GameWindow::sendDrawingPoints(const QVector<QPoint>& points)
{
    if (!m_isDrawing) return;

    QJsonArray pointsArray;
    for (const QPoint& p : points) {
        pointsArray.append(QJsonObject{{"x", p.x()}, {"y", p.y()}});
    }

    QJsonObject message;
    message["type"] = "draw";
    message["points"] = pointsArray;

    sendJson(message);
}

void GameWindow::sendJson(const QJsonObject &message)
{
    QByteArray jsonData = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << (quint32)jsonData.size();
    block.append(jsonData);

    m_socket->write(block);
    m_socket->flush();
}

void GameWindow::on_sendGuessButton_clicked()
{
    QString guess = ui->guessEdit->text().trimmed();
    if (guess.isEmpty()) return;

    QJsonObject message;
    message["type"] = "guess";
    message["text"] = guess;
    message["player"] = m_playerName;

    sendJson(message);
    ui->guessEdit->clear();
}
