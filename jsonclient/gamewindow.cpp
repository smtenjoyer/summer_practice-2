#include "gamewindow.h"
#include "ui_gamewindow.h"
#include <QJsonDocument>
#include "DoodleArea.h"
#include <QLayout>

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

    QSize newSize(1121, 711);
    m_doodleArea = new DoodleArea(newSize);
    m_doodleArea->resize(newSize);

    QLayout * layout = ui->drawingAreaContainer->layout();
    if (layout == nullptr) {
        layout = new QVBoxLayout;
        ui->drawingAreaContainer->setLayout(layout);
    }

    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    layout->addWidget(m_doodleArea);

    //Работает Киря не прикасаться
    // Новые соединения
    connect(this, &GameWindow::sendDrawingCommand, this, [this](const QJsonObject& cmd) {
        if (m_isDrawing) {
            QJsonObject message = cmd;
            message["type"] = "draw";
            emit sendMessage(message);
        }
    });

    connect(m_doodleArea, &DoodleArea::drawingCommandGenerated,
            this, &GameWindow::sendDrawingCommand);
    //
    qDebug() << "Player:" << m_playerName << "isDrawing:" << m_isDrawing;
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
    ui->guessEdit->clear();
}

void GameWindow::updateScoresTable(const QJsonObject& scores) {
    for (int row = 0; row < (ui->scoresTable->rowCount()); ++row) {
        QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
        if(!nameItem) continue; //Проверяем, что элемент существует
        QString playerName = nameItem->text();

        if (scores.contains(playerName)) {
            int score = scores[playerName].toInt(); // Получаем счет игрока
            QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);
            if (scoreItem) {
                scoreItem->setText(QString::number(score)); // Обновляем счет в таблице
            } else {
                scoreItem = new QTableWidgetItem(QString::number(score));
                ui->scoresTable->setItem(row, 1, scoreItem);
            }
        }
    }
}

void GameWindow::processServerMessage(const QJsonObject &message) {
    QString type = message["type"].toString();
    // qDebug() << "Processing message type:" << type;  // Логирование для отладки


    if (type == "playerJoined") {
        QString Name = message["name"].toString();
        QJsonObject scores = message["scores"].toObject();

        // Поиск игрока в таблице
        int playerRow = -1;
        for (int row = 0; row < ui->scoresTable->rowCount(); ++row) {
            QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
            if (nameItem && nameItem->text() == Name) {
                playerRow = row;
                break;
            }
        }

        // Если игрок не найден, добавляем новую строку
        if (playerRow == -1) {
            int rowCount = ui->scoresTable->rowCount();

            // **Ключевое изменение: Сначала создаем строку!**
            ui->scoresTable->insertRow(rowCount);
            playerRow = rowCount; // Сохраняем индекс новой строки
        }

        // Создаем элементы для имени и очков (с нулевым значением)
        QTableWidgetItem *item1 = new QTableWidgetItem(Name);
        QTableWidgetItem *item2 = new QTableWidgetItem("0"); // Инициализируем счет нулем.

        // Вставляем элементы в таблицу
        ui->scoresTable->setItem(playerRow, 0, item1);
        ui->scoresTable->setItem(playerRow, 1, item2);


        // Глобальный метод обновления очков:
        for (int row = 0; row < ui->scoresTable->rowCount(); ++row) {
            QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
            if (nameItem) {
                QString playerName = nameItem->text();

                // Проверяем, есть ли для этого игрока очки в полученном сообщении
                if (scores.contains(playerName)) {
                    int score = scores[playerName].toInt();

                    // Получаем элемент таблицы для очков
                    QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);

                    // Обновляем или создаем элемент таблицы с очками
                    if (scoreItem) {
                        scoreItem->setText(QString::number(score));
                    } else {
                        scoreItem = new QTableWidgetItem(QString::number(score));
                        ui->scoresTable->setItem(row, 1, scoreItem);
                    }
                } else {
                    // Если в сообщении нет очков для этого игрока, сбрасываем их в 0 (опционально)
                    QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);
                    if (scoreItem) {
                        scoreItem->setText("0");
                    }
                }
            }
        }
    }



    else if (type == "roundStart") {
        QString drawer = message["drawer"].toString();
        m_isDrawing = (drawer == m_playerName);

        // Всегда очищаем холст при начале раунда
        m_doodleArea->clearImage();

        setupGameUI(m_isDrawing);

        if (!m_isDrawing) {
            ui->wordLabel->setText("Угадайте что рисует " + drawer);
        }
    }
    else if (type == "yourTurn") {
        QString word = message["word"].toString();
        ui->wordLabel->setText("Нарисуй-ка мне " + word);
    }
    else if (type == "draw") {
        // Принимаем все команды, кроме случаев когда мы художник И это не команда очистки
        if (!m_isDrawing || message["tool"] == "clear") {
            m_doodleArea->applyRemoteCommand(message);
        }
    }
    else if (type == "chat") {
        QString player = message["player"].toString();
        QString text = message["text"].toString();
        ui->chatText->append(player + ": " + text);
    }
    else if (type == "correctGuess") {
        QJsonObject scores = message["scores"].toObject();
        updateScoresTable(scores);
        QString guesser = message["guesser"].toString();
        QString word = message["word"].toString();
        ui->chatText->append("✓ " + guesser + " угадал: " + word);

        // Автоматически очищаем поле ввода
        ui->guessEdit->clear();
    }

    else {
        qDebug() << "Unknown message type:" << type;
    }
}

void GameWindow::setupGameUI(bool isDrawer){

    if (!isDrawer){
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
    emit sendMessage(message);
}


