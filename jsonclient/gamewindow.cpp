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

    // Изначальная настройка UI (видимость кнопок и полей)
    setupGameUI(false);

    // --- Инициализация и настройка DoodleArea ---
    QSize doodleAreaSize(1121, 711); // Используем явно заданный размер для холста
    m_doodleArea = new DoodleArea(doodleAreaSize, this); // Создаем экземпляр DoodleArea, передавая GameWindow как родителя


    // setActions(m_isDrawing);
    setNoneTool(); // Если не художник, никаких инструментов
    ui->toolBar->setHidden(true);


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

    createActions();
    createToolBars();
    setWindowTitle(tr("Крокодил"));

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
        qDebug() << "CLIENT (" << m_playerName << "): Processing 'playerJoined' for:" << Name << ". Full scores received:" << scores;

        // Вместо всей предыдущей логики обновления таблицы:
        updateAllPlayersTable(scores); // Используем новую унифицированную функцию
    }
    // НОВЫЙ БЛОК: Обработка полного списка игроков при подключении
    else if (type == "playerList") { // Или "initialState", как решите на сервере
        QJsonArray playersArray = message["players"].toArray();
        qDebug() << "CLIENT (" << m_playerName << "): Processing 'playerList'. Players count:" << playersArray.size();

        QJsonObject scoresFromList;
        for (const QJsonValue& value : playersArray) {
            QJsonObject playerObj = value.toObject();
            scoresFromList[playerObj["name"].toString()] = playerObj["score"].toInt();
        }
        updateAllPlayersTable(scoresFromList); // Преобразуем и обновляем
        qDebug() << "CLIENT (" << m_playerName << "): Получен и обновлен полный список игроков.";
    }
    else if (type == "roundStart") {
        QString drawer = message["drawer"].toString();
        m_isDrawing = (drawer == m_playerName); // Определяем, является ли текущий игрок художником
        setActions(m_isDrawing);

        m_doodleArea->clearImage(); // Всегда очищаем холст при начале раунда

        setupGameUI(m_isDrawing); // Настраиваем UI в зависимости от роли


        if (!m_isDrawing) {
            ui->wordLabel->setText("Угадайте что рисует " + drawer);

        } else {
            setPencilTool(); // Если художник, по умолчанию карандаш
            ui->toolBar->setVisible(true);
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
        setActions(m_isDrawing);
        setNoneTool(); // Если не художник, никаких инструментов
        ui->toolBar->setHidden(true);


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

    // Угадывание для НЕ-художника
    ui->guessEdit->setEnabled(!isDrawer);
    ui->sendGuessButton->setEnabled(!isDrawer);

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



void GameWindow::createActions(){

    clearScreenAct = new QAction(tr("&Очистить изображение..."), this);
    clearScreenAct->setShortcut(tr("Ctrl+L"));
    connect(clearScreenAct, SIGNAL(triggered()), m_doodleArea, SLOT(clearImage()));



    penColorAct = new QAction(QIcon(":/images/Color.png"), "Цвет", this);
    connect(penColorAct, SIGNAL(triggered()), this, SLOT(penColor()));
    penWidthAct = new QAction(QIcon(":/images/Width.png"), "Толщина линии", this);
    connect(penWidthAct, SIGNAL(triggered()), this, SLOT(penWidth()));



    fillAreaAct = new QAction(QIcon(":/images/fill.png"), "Заливка", this);
    connect(fillAreaAct, SIGNAL(triggered()), this, SLOT(setFillTool()));

    PencilAct = new QAction(QIcon(":/images/Pencil.png"), "Карандаш", this);
    RubberAct = new QAction(QIcon(":/images/Rubber.png"), "Ластик", this);

    lineAction = new QAction(QIcon(":/images/Line.png"), tr("&Прямая"), this);
    rectangleAction = new QAction(QIcon(":/images/Rectangle.png"), tr("&Прямоугольник"), this);
    ellipseAction = new QAction(QIcon(":/images/Ellipse.png"), tr("&Эллипс"), this);



    undoActionBtn = new QAction(tr("&U"), this);
    undoActionBtn->setShortcut(QKeySequence::Undo);
    connect(undoActionBtn, &QAction::triggered, this, &GameWindow::undoAction);

    redoActionBtn = new QAction(tr("&R"), this);
    redoActionBtn->setShortcut(QKeySequence::Redo);
    connect(redoActionBtn, &QAction::triggered, this, &GameWindow::redoAction);

    QActionGroup *toolGroup = new QActionGroup(this);
    toolGroup->addAction(PencilAct);
    toolGroup->addAction(RubberAct);
    toolGroup->addAction(fillAreaAct);
    toolGroup->addAction(lineAction);
    toolGroup->addAction(rectangleAction);
    toolGroup->addAction(ellipseAction);
    toolGroup->addAction(undoActionBtn);
    toolGroup->addAction(redoActionBtn);

    PencilAct->setCheckable(true);
    RubberAct->setCheckable(true);
    fillAreaAct->setCheckable(true);
    lineAction->setCheckable(true);
    rectangleAction->setCheckable(true);
    ellipseAction->setCheckable(true);

    PencilAct->setChecked(true);

    connect(PencilAct, SIGNAL(triggered()), this, SLOT(setPencilTool()));
    connect(RubberAct, SIGNAL(triggered()), this, SLOT(setRubberTool()));
    connect(fillAreaAct, SIGNAL(triggered()), this, SLOT(setFillTool()));
    connect(lineAction, &QAction::triggered, this, &GameWindow::setLineTool);
    connect(rectangleAction, &QAction::triggered, this, &GameWindow::setRectangleTool);
    connect(ellipseAction, &QAction::triggered, this, &GameWindow::setEllipseTool);


    undoActionBtn->setShortcut(tr("Ctrl+Z"));
    redoActionBtn->setShortcut(tr("Ctrl+Y"));
}


void GameWindow::createToolBars() {

    addToolBar(Qt::RightToolBarArea, ui->toolBar);

    ui->toolBar->addAction(PencilAct);
    ui->toolBar->addAction(RubberAct);
    ui->toolBar->addAction(fillAreaAct);
    ui->toolBar->addAction(lineAction);
    ui->toolBar->addAction(rectangleAction);
    ui->toolBar->addAction(ellipseAction);

    ui->toolBar->addSeparator();
    ui->toolBar->addAction(penColorAct);
    ui->toolBar->addAction(penWidthAct);

    ui->toolBar->addSeparator();
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(undoActionBtn);
    ui->toolBar->addAction(redoActionBtn);
}


void GameWindow::updateBrushPreview(QLabel *label, int width, QColor color) {
    int size = 60;
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QPoint center(size / 2, size / 2);
    painter.drawPoint(center);

    painter.end();

    label->setPixmap(QPixmap::fromImage(image)); // Преобразуем QImage в QPixmap
}


void GameWindow::penColor(){
    QColor newColor = QColorDialog::getColor((m_doodleArea->penColor()));
    if(newColor.isValid()){
        m_doodleArea->setPenColor(newColor);
    }
}



void GameWindow::penWidth() {
    QDialog dialog(this);
    dialog.setFixedSize(200,100);
    dialog.setWindowTitle(tr("cooler_paint - Толщина кисти"));

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 50);
    slider->setValue(m_doodleArea->penWidth());

    QLabel *previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignCenter);
    updateBrushPreview(previewLabel, m_doodleArea->penWidth(), m_doodleArea->penColor()); // Изначальное отображение

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(slider);
    layout->addWidget(previewLabel);

    QObject::connect(slider, &QSlider::valueChanged, this, [=](int value) {
        updateBrushPreview(previewLabel, value, m_doodleArea->penColor());
    });

    QObject::connect(slider, &QSlider::valueChanged, m_doodleArea, &DoodleArea::setPenWidth);

    dialog.exec();
}

void GameWindow::setActions(const bool &drawing){

    penColorAct->setEnabled(drawing);
    penWidthAct->setEnabled(drawing);
    clearScreenAct->setEnabled(drawing);
    fillAreaAct->setEnabled(drawing);
    PencilAct->setEnabled(drawing);
    RubberAct->setEnabled(drawing);
    lineAction->setEnabled(drawing);
    rectangleAction->setEnabled(drawing);
    ellipseAction->setEnabled(drawing);

    undoActionBtn->setEnabled(drawing);
    redoActionBtn->setEnabled(drawing);

}

void GameWindow::updateAllPlayersTable(const QJsonObject& scores) {
    ui->scoresTable->setRowCount(0); // Полностью очищаем таблицу


    QList<QPair<QString, int>> playersData;
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        playersData.append({it.key(), it.value().toInt()});
    }

    // Опционально: отсортировать игроков по очкам или имени
     std::sort(playersData.begin(), playersData.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b){
         return a.first < b.first; // Сортировка по имени
     });

    ui->scoresTable->setRowCount(playersData.size()); // Устанавливаем нужное количество строк

    for (int i = 0; i < playersData.size(); ++i) {
        QTableWidgetItem *nameItem = new QTableWidgetItem(playersData[i].first);
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(playersData[i].second));

        ui->scoresTable->setItem(i, 0, nameItem);
        ui->scoresTable->setItem(i, 1, scoreItem);
    }
    qDebug() << "CLIENT (" << m_playerName << "): Scores table fully updated.";
}
