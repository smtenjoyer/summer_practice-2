
#include "gamewindow.h"
#include "ui_gamewindow.h"
#include <QJsonDocument>
#include "DoodleArea.h"

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

    setupGameUI(false);

    QSize newSize(800, 600);
    m_doodleArea = new DoodleArea(newSize);
    setCentralWidget(m_doodleArea);
    // connect(m_doodleArea, &DoodleArea::pointsUpdated, this, &GameWindow::sendDrawingPoints);
}

GameWindow::~GameWindow()
{
    delete ui;
}

void GameWindow::on_sendGuessButton_clicked()
{
    QString guess = ui->guessEdit->text().trimmed();
    if (guess.isEmpty()) return;

    QJsonObject message;
    message["type"] = "guess";
    message["text"] = guess;
    emit sendMessage(message);

    QJsonDocument doc(message);
    m_socket->write(doc.toJson());
    m_socket->flush();
    ui->guessEdit->clear();
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

            // Отрисовка полученных точек
            //Check it Doodle Area name
            //if (!points.isEmpty()) {
            //m_doodleArea->drawPoints(points); // Предполагается, что у вас есть такой метод
            //}
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
        // ui->scoresTable->clear();

        // ... заполнение таблицы очков ...
    }
}

void GameWindow::setupGameUI(bool isDrawer){
    // ui->drawingWidget->setVisible(isDrawer);
    // ui->guessWidget->setVisible(!isDrawer);

    if (isDrawer){
        ui->wordLabel->setText("Ваш ход рисовать!");
    } else {
        ui->wordLabel->setText("Угадывайте что рисуют!");
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

    QJsonDocument doc(message);
    m_socket->write(doc.toJson());
}


