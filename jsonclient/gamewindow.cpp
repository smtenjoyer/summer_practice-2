#include "gamewindow.h"
#include "ui_gamewindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpSocket>
#include <QDebug>
#include <QLayout> // Для работы с QLayout
#include <QVBoxLayout> // Для использования QVBoxLayout
#include <QSlider> // Для QSlider
#include <QPushButton> // Для QPushButton, если не было
#include <QLabel> // Для QLabel, если не было
#include <QTableWidgetItem> // Для QTableWidgetItem

// Убедитесь, что DoodleArea.h находится по правильному пути
#include "DoodleArea.h"

// Конструктор GameWindow
GameWindow::GameWindow(QTcpSocket* socket, const QString& playerName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GameWindow),
    m_socket(socket),
    m_playerName(playerName),
    m_isDrawing(false),
    m_doodleArea(nullptr) // Инициализируем указатель члена класса
{
    ui->setupUi(this); // Загружаем UI из .ui файла

    m_socket->setParent(this); // Устанавливаем GameWindow родителем для сокета

    // --- Подключение кнопок инструментов ---
    connect(ui->pencilToolButton, &QPushButton::clicked, this, &GameWindow::setPencilTool);
    connect(ui->rubberToolButton, &QPushButton::clicked, this, &GameWindow::setRubberTool);
    connect(ui->fillToolButton, &QPushButton::clicked, this, &GameWindow::setFillTool);
    connect(ui->lineToolButton, &QPushButton::clicked, this, &GameWindow::setLineTool);
    connect(ui->rectangleToolButton, &QPushButton::clicked, this, &GameWindow::setRectangleTool);
    connect(ui->ellipseToolButton, &QPushButton::clicked, this, &GameWindow::setEllipseTool);

    connect(ui->undoButton, &QPushButton::clicked, this, &GameWindow::undoAction);
    connect(ui->redoButton, &QPushButton::clicked, this, &GameWindow::redoAction);

    // --- Подключение кнопки отправки догадки ---
    connect(ui->sendGuessButton, &QPushButton::clicked, this, &GameWindow::on_sendGuessButton_clicked);

    // Изначальная настройка UI (видимость кнопок и полей)
    setupGameUI(false);

    // --- Инициализация и настройка DoodleArea ---
    QSize doodleAreaSize(1121, 711); // Используем явно заданный размер для холста
    m_doodleArea = new DoodleArea(doodleAreaSize, this); // Создаем экземпляр DoodleArea, передавая GameWindow как родителя
    setNoneTool(); // Устанавливаем инструмент по умолчанию "None"

    // --- Настройка слайдера толщины пера ---
    QSlider *penWidthSlider = ui->horizontalSliderPenWidth; // Получаем указатель на слайдер из UI
    if (penWidthSlider) { // Проверяем, что слайдер найден
        // Устанавливаем диапазон для слайдера (от 1 до 50 пикселей)
        penWidthSlider->setRange(1, 50);
        // Устанавливаем начальное значение слайдера на текущую толщину пера в DoodleArea
        // (предполагается, что DoodleArea::getPenWidth() существует)
        if (m_doodleArea) {
            penWidthSlider->setValue(m_doodleArea->getPenWidth());
        }
        // Подключаем сигнал valueChanged слайдера к слоту setPenWidth в DoodleArea
        connect(penWidthSlider, &QSlider::valueChanged, m_doodleArea, &DoodleArea::setPenWidth);
    } else {
        qWarning() << "Ошибка: Слайдер 'horizontalSliderPenWidth' не найден в UI. Проверьте objectName.";
    }

    // --- Добавление DoodleArea в контейнер UI ---
    QLayout *layout = ui->drawingAreaContainer->layout();
    if (layout == nullptr) { // Если у контейнера нет лейаута, создаем QVBoxLayout
        layout = new QVBoxLayout;
        ui->drawingAreaContainer->setLayout(layout);
    }

    // Очищаем контейнер от существующих виджетов (если они есть), чтобы избежать наложения
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget(); // Удаляем виджет
        delete item; // Удаляем элемент лейаута
    }
    layout->addWidget(m_doodleArea); // Добавляем наш DoodleArea в лейаут

    // --- Соединения для отправки и получения команд рисования ---
    // Этот лямбда-слот перехватывает команду рисования от GameWindow и отправляет ее через сокет
    connect(this, &GameWindow::sendDrawingCommand, this, [this](const QJsonObject& cmd) {
        // Отправляем команду только если текущий игрок является художником
        if (m_isDrawing) {
            QJsonObject message = cmd;
            message["type"] = "draw"; // Убеждаемся, что тип сообщения "draw"
            emit sendMessage(message); // Отправляем сообщение через сокет
        }
    });

    // Это соединение передает команды, сгенерированные DoodleArea, в GameWindow для дальнейшей отправки
    connect(m_doodleArea, &DoodleArea::drawingCommandGenerated,
            this, &GameWindow::sendDrawingCommand);

    qDebug() << "Player:" << m_playerName << "isDrawing:" << m_isDrawing;
}

// Деструктор GameWindow
GameWindow::~GameWindow()
{
    // m_doodleArea удалится автоматически, так как GameWindow является его родителем
    delete ui; // Удаляем UI-объект
}

// --- Слоты для кнопок инструментов ---
void GameWindow::setPencilTool() {
    if (m_doodleArea) {
        m_doodleArea->setTool(DoodleArea::Pencil);
        m_doodleArea->setCursor(Qt::CrossCursor); // Меняем курсор на крестик
        qDebug() << "Tool set to Pencil";
    }
}

void GameWindow::setRubberTool() {
    if (m_doodleArea) {
        m_doodleArea->setTool(DoodleArea::Rubber);
        m_doodleArea->setCursor(Qt::CrossCursor); // Меняем курсор на крестик
        qDebug() << "Tool set to Rubber";
    }
}

void GameWindow::setFillTool() {
    if (m_doodleArea) {
        m_doodleArea->setTool(DoodleArea::Fill);
        m_doodleArea->setCursor(Qt::CrossCursor);
        qDebug() << "Tool set to Fill";
    }
}

void GameWindow::setLineTool() {
    if(m_doodleArea){
        m_doodleArea->setTool(DoodleArea::Line);
        m_doodleArea->setCursor(Qt::CrossCursor);
        qDebug() << "Tool set to Line";
    }
}

void GameWindow::setRectangleTool() {
    if(m_doodleArea){
        m_doodleArea->setTool(DoodleArea::Rectangle);
        m_doodleArea->setCursor(Qt::CrossCursor);
        qDebug() << "Tool set to Rectangle";
    }
}

void GameWindow::setEllipseTool() {
    if(m_doodleArea){
        m_doodleArea->setTool(DoodleArea::Ellipse);
        m_doodleArea->setCursor(Qt::CrossCursor);
        qDebug() << "Tool set to Ellipse";
    }
}

void GameWindow::setNoneTool() {
    if (m_doodleArea) {
        m_doodleArea->setTool(DoodleArea::None);
        m_doodleArea->setCursor(Qt::ArrowCursor); // Стандартный курсор для "None"
        qDebug() << "Tool set to None";
    }
}

void GameWindow::undoAction() {
    if (m_doodleArea) {
        m_doodleArea->undo();
        qDebug() << "Undo action triggered";
    }
}

void GameWindow::redoAction() {
    if (m_doodleArea) {
        m_doodleArea->redo();
        qDebug() << "Redo action triggered";
    }
}

// --- Слот для отправки догадки ---
void GameWindow::on_sendGuessButton_clicked()
{
    QString guess = ui->guessEdit->text().trimmed();
    if (guess.isEmpty()) return;

    QJsonObject message;
    message["type"] = "guess";
    message["text"] = guess;
    emit sendMessage(message); // Отправляем догадку

    ui->guessEdit->clear(); // Очищаем поле ввода после отправки
}

// --- Обновление таблицы очков ---
void GameWindow::updateScoresTable(const QJsonObject& scores) {
    for (int row = 0; row < (ui->scoresTable->rowCount()); ++row) {
        QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
        if(!nameItem) continue; // Проверяем, что элемент существует
        QString playerName = nameItem->text();

        if (scores.contains(playerName)) {
            int score = scores[playerName].toInt();
            QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);
            if (scoreItem) {
                scoreItem->setText(QString::number(score)); // Обновляем счет
            } else {
                scoreItem = new QTableWidgetItem(QString::number(score));
                ui->scoresTable->setItem(row, 1, scoreItem);
            }
        }
    }
}

// --- Обработка сообщений от сервера ---
void GameWindow::processServerMessage(const QJsonObject &message) {
    QString type = message["type"].toString();
    // qDebug() << "Processing message type:" << type;

    if (type == "playerJoined") {
        QString Name = message["name"].toString();
        QJsonObject scores = message["scores"].toObject();

        int playerRow = -1;
        for (int row = 0; row < ui->scoresTable->rowCount(); ++row) {
            QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
            if (nameItem && nameItem->text() == Name) {
                playerRow = row;
                break;
            }
        }

        if (playerRow == -1) {
            int rowCount = ui->scoresTable->rowCount();
            ui->scoresTable->insertRow(rowCount);
            playerRow = rowCount;
        }

        QTableWidgetItem *item1 = new QTableWidgetItem(Name);
        QTableWidgetItem *item2 = new QTableWidgetItem("0");
        ui->scoresTable->setItem(playerRow, 0, item1);
        ui->scoresTable->setItem(playerRow, 1, item2);

        // Глобальный метод обновления очков (для всех игроков, включая только что присоединившегося)
        for (int row = 0; row < ui->scoresTable->rowCount(); ++row) {
            QTableWidgetItem* nameItem = ui->scoresTable->item(row, 0);
            if (nameItem) {
                QString playerName = nameItem->text();
                if (scores.contains(playerName)) {
                    int score = scores[playerName].toInt();
                    QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);
                    if (scoreItem) {
                        scoreItem->setText(QString::number(score));
                    } else {
                        scoreItem = new QTableWidgetItem(QString::number(score));
                        ui->scoresTable->setItem(row, 1, scoreItem);
                    }
                } else {
                    QTableWidgetItem* scoreItem = ui->scoresTable->item(row, 1);
                    if (scoreItem) {
                        scoreItem->setText("0"); // Если игрока нет в текущих очках, сбросить
                    }
                }
            }
        }
    }
    else if (type == "roundStart") {
        QString drawer = message["drawer"].toString();
        m_isDrawing = (drawer == m_playerName); // Определяем, является ли текущий игрок художником

        m_doodleArea->clearImage(); // Всегда очищаем холст при начале раунда

        setupGameUI(m_isDrawing); // Настраиваем UI в зависимости от роли

        if (!m_isDrawing) {
            ui->wordLabel->setText("Угадайте что рисует " + drawer);
            setNoneTool(); // Если не художник, никаких инструментов
        } else {
            setPencilTool(); // Если художник, по умолчанию карандаш
        }
    }
    else if (type == "yourTurn") {
        QString word = message["word"].toString();
        ui->wordLabel->setText("Нарисуй-ка мне " + word);
    }
    else if (type == "draw") {
        // Принимаем и применяем все команды рисования, кроме тех, что мы сами генерируем (если мы художник)
        // Исключение: команды очистки всегда применяются, независимо от роли.
        if (!m_isDrawing || message["tool"].toString() == "clear") {
            m_doodleArea->applyRemoteCommand(message);
        }
    }
    else if (type == "chat") {
        QString player = message["player"].toString();
        QString text = message["text"].toString();
        ui->chatText->append(player + ": " + text); // Добавляем сообщение в чат
    }
    else if (type == "correctGuess") {
        QJsonObject scores = message["scores"].toObject();
        updateScoresTable(scores); // Обновляем таблицу очков
        QString guesser = message["guesser"].toString();
        QString word = message["word"].toString();
        ui->chatText->append("✓ " + guesser + " угадал: " + word); // Сообщение об угадывании

        ui->guessEdit->clear(); // Очищаем поле ввода догадок
    }
    else if (type == "roundEnd") {
        setNoneTool(); // Отключаем инструменты
        ui->wordLabel->setText("Приготовились!"); // Сообщение о конце раунда
        QJsonObject scores = message["scores"].toObject();
        updateScoresTable(scores); // Обновляем очки
    }
    else if (type == "gameOver") {
        QJsonObject scores = message["scores"].toObject();
        processGameOver(scores); // Обрабатываем окончание игры
    }
    else {
        qDebug() << "Unknown message type:" << type;
    }
}

// --- Обработка окончания игры ---
void GameWindow::processGameOver(const QJsonObject& scores) {
    QList<QPair<QString, int>> sortedScores;
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        QString playerName = it.key();
        int playerScore = it.value().toInt();
        sortedScores.append(qMakePair(playerName, playerScore));
    }

    // Сортируем игроков по очкам (по убыванию)
    std::sort(sortedScores.begin(), sortedScores.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
        return a.second > b.second;
    });

    // Получаем виджет страницы окончания игры (предполагается, что это вторая страница в stackedWidget)
    QWidget* gameOverPage = ui->stackedWidget->widget(1);
    // Находим QLabel'ы для отображения мест и очков
    QLabel* firstPlace = gameOverPage->findChild<QLabel*>("firstPlace");
    QLabel* secondPlace = gameOverPage->findChild<QLabel*>("secondPlace");
    QLabel* thirdPlace = gameOverPage->findChild<QLabel*>("thirdPlace");
    QLabel* numFirst = gameOverPage->findChild<QLabel*>("numFirst");
    QLabel* numSecond = gameOverPage->findChild<QLabel*>("numSecond");
    QLabel* numThird = gameOverPage->findChild<QLabel*>("numThird");

    // Заполняем места победителей
    if (firstPlace && numFirst) {
        if (sortedScores.size() > 0) {
            firstPlace->setText(sortedScores[0].first);
            numFirst->setText(QString::number(sortedScores[0].second));
        } else {
            firstPlace->setText("Нет данных");
            numFirst->setText("0");
        }
    }

    if (secondPlace && numSecond) {
        if (sortedScores.size() > 1) {
            secondPlace->setText(sortedScores[1].first);
            numSecond->setText(QString::number(sortedScores[1].second));
        } else {
            secondPlace->setText("Нет данных");
            numSecond->setText("0");
        }
    }

    if (thirdPlace && numThird) {
        if (sortedScores.size() > 2) {
            thirdPlace->setText(sortedScores[2].first);
            numThird->setText(QString::number(sortedScores[2].second));
        } else {
            thirdPlace->setText("Нет данных");
            numThird->setText("0");
        }
    }

    ui->stackedWidget->setCurrentIndex(1); // Переключаемся на страницу окончания игры
}

// --- Настройка UI в зависимости от роли игрока (художник/угадывающий) ---
void GameWindow::setupGameUI(bool isDrawer){
    // Включаем/отключаем кнопки инструментов
    ui->pencilToolButton->setEnabled(isDrawer);
    ui->rubberToolButton->setEnabled(isDrawer);
    ui->fillToolButton->setEnabled(isDrawer);
    ui->lineToolButton->setEnabled(isDrawer);
    ui->rectangleToolButton->setEnabled(isDrawer);
    ui->ellipseToolButton->setEnabled(isDrawer);
    // ui->textToolButton->setEnabled(isDrawer); // Если вы когда-нибудь добавите текст

    ui->undoButton->setEnabled(isDrawer);
    ui->redoButton->setEnabled(isDrawer);

    // Угадывание для НЕ-художника
    ui->guessEdit->setEnabled(!isDrawer);
    ui->sendGuessButton->setEnabled(!isDrawer);
    ui->horizontalSliderPenWidth->setEnabled(isDrawer); // Включаем/отключаем слайдер

    if (!isDrawer){
        ui->wordLabel->setText("Угадывайте что рисуют!");
    } else {
        // Установка инструмента по умолчанию для художника (карандаш)
        // Это уже делается в processServerMessage при roundStart, но можно продублировать
        setPencilTool();
    }
}

// Эта функция, похоже, не используется сейчас, но оставлена на случай, если понадобится
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
