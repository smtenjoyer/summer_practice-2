#include "myserver.h"
#include <QList>

myserver::myserver(QObject *parent) : QTcpServer(parent),
    m_gameState(WaitingForPlayers),
    m_currentRound(0)
{
    m_words = QStringList()  = {"Крокодил", "Самолет", "Малыш Йода", "Яблоко", "Программист", "Слон"};
    connect(&m_roundTimer, &QTimer::timeout, this, &myserver::onRoundTimerTimeout);
}

myserver::~myserver()
{
    for (QTcpSocket* socket : m_clients){
        socket->close();
        socket->deleteLater();
    }
}

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
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &myserver::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &myserver::onDisconnected);

    m_clients.append(socket);

    qDebug()<<socketDescriptor<<" Client connected";

    // Если игра уже идет, отправляем новому клиенту историю рисования
    if (m_gameState == Drawing && !m_drawingHistory.isEmpty()) {
        qDebug() << "Sending drawing history to new client";
        for (const QJsonObject& cmd : m_drawingHistory) {
            sendToClient(socket, cmd);
        }
    }
}

void myserver::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    m_data.append(socket->readAll());

    while (m_data.contains("\n")){
        int pos = m_data.indexOf("\n");
        QByteArray messageData = m_data.left(pos);
        m_data.remove(0, pos + 1);

        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (doc.isObject()){
            processMessage(doc.object(), socket);
        }
    }
}

void myserver::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    QString playerName = m_clientNames.value(socket, "");
    if (!playerName.isEmpty()){
        m_clientNames.remove(socket);
        m_scores.remove(playerName);

        QJsonObject message;
        message["type"] = "playerLeft";
        message["player"] = playerName;
        broadcast(message);
    }

    m_clients.removeOne(socket);
    socket->deleteLater();

    qDebug()<<"Client disconnect";
}
void myserver::processMessage(const QJsonObject &message, QTcpSocket *sender) {
    QString type = message["type"].toString();
    QString senderName = m_clientNames.value(sender, "");
    qDebug() << "Message from" << senderName << ":" << message;

    if (type == "register") {
        QString name = message["name"].toString();
        m_clientNames[sender] = name;
        m_scores[name] = 0;

        QJsonObject response;
        response["type"] = "registered";
        response["success"] = true;
        sendToClient(sender, response);

        QJsonObject playerJoined;
        playerJoined["type"] = "playerJoined";
        playerJoined["name"] = name;

        QJsonObject scoresObject;
        for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
            scoresObject[it.key()] = it.value();
        }
        playerJoined["scores"] = scoresObject;
        broadcast(playerJoined);

        //  Отправка полного списка игроков новому клиенту
        QJsonObject playerListMsg;
        playerListMsg["type"] = "playerList";
        QJsonArray playersArray;
        for (QTcpSocket* clientSocket : m_clients) {
            if (m_clientNames.contains(clientSocket)) {
                QString existingPlayerName = m_clientNames.value(clientSocket);
                QJsonObject playerObj;
                playerObj["name"] = existingPlayerName;
                playerObj["score"] = m_scores.value(existingPlayerName, 0);
                playersArray.append(playerObj);
            }
        }
        playerListMsg["players"] = playersArray;
        sendToClient(sender, playerListMsg);
        if (m_gameState == WaitingForPlayers && m_clientNames.size() >= 2) {                      //!!!!!!!
            startGame();
        }
    }
    else if(type == "draw") {
        if (m_isRoundActive && senderName == m_currentDrawer) {
            m_drawingHistory.append(message);
            broadcast(message);
            qDebug() << "Draw command from" << senderName << "in round" << m_currentRound;
        }
        else {
            qDebug() << "Draw command rejected. Round active:" << m_isRoundActive
                     << "Is drawer:" << (senderName == m_currentDrawer);
        }
    }
    else if (type == "guess") {
        if (m_gameState == Drawing && senderName != m_currentDrawer) {
            QString guess = message["text"].toString().trimmed().toLower();

            if (guess.isEmpty()) {
                qDebug() << "Empty guess from" << senderName;
                return;
            }

            if (guess == m_currentWord.toLower()) {
                // Правильный ответ
                QString guesser = senderName;
                m_scores[guesser] += 10;
                m_scores[m_currentDrawer] += 5;

                QJsonObject correctGuess;
                correctGuess["type"] = "correctGuess";
                correctGuess["guesser"] = guesser;
                correctGuess["word"] = m_currentWord;
                correctGuess["drawer"] = m_currentDrawer;  // Добавлено для информации

                // Формируем обновленные очки
                QJsonObject scoresObject;
                for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
                    scoresObject[it.key()] = it.value();
                }
                correctGuess["scores"] = scoresObject;

                broadcast(correctGuess);
                endRound();
                ifOver();
            }
            else {
                // Неправильный ответ
                QJsonObject chatMessage;
                chatMessage["type"] = "chat";
                chatMessage["player"] = senderName;
                chatMessage["text"] = message["text"].toString();  // Оригинальный текст (без toLower)
                broadcast(chatMessage);
            }
        }
    }
    else {
        qDebug() << "Unknown message type received:" << type;
    }
}

void myserver::startGame(){
    m_gameState = Drawing;
    m_isRoundActive = true;
    m_currentRound = 1;
    startNewRound();
}



void myserver::startNewRound() {
    m_gameState = Drawing;
    m_isRoundActive = true; // Раунд активен
    m_drawingHistory.clear();

    selectNewDrawer();
    m_currentWord = selectRandomWord();

    // Отправляем слово только художнику
    QTcpSocket* drawerSocket = m_clientNames.key(m_currentDrawer);
    if (drawerSocket) {
        QJsonObject drawerMsg;
        drawerMsg["type"] = "yourTurn";
        drawerMsg["word"] = m_currentWord;
        sendToClient(drawerSocket, drawerMsg);
    }

    // Уведомляем всех о начале раунда
    QJsonObject roundStart;
    roundStart["type"] = "roundStart";
    roundStart["drawer"] = m_currentDrawer;
    roundStart["round"] = m_currentRound;
    broadcast(roundStart);

    // Явная команда очистки всем
    QJsonObject clearCmd;
    clearCmd["type"] = "draw";
    clearCmd["tool"] = "clear";
    broadcast(clearCmd);

    m_roundTimer.start(60000);
}


void myserver::endRound() {
    m_isRoundActive = false; // Раунд завершен
    m_roundTimer.stop();
    m_gameState = RoundEnd;

    QJsonObject roundEnd;
    roundEnd["type"] = "roundEnd";

    QJsonObject scoresObject;
    for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
        scoresObject[it.key()] = it.value();
    }
    roundEnd["scores"] = scoresObject;
    broadcast(roundEnd);

    // Сброс состояния для следующего раунда
    m_currentWord = ""; // Очищаем слово
    m_drawingHistory.clear(); // Очищаем историю рисования
    // m_currentDrawer - оставляем, чтобы он не смог рисовать в начале нового раунда
    // m_gameState = WaitingForPlayers; // Оставляем, ждем пока пройдет таймаут
    QTimer::singleShot(5000, this, &myserver::startNewRound); // Запускаем новый раунд через 5 секунд
}

void myserver::selectNewDrawer() {
    if (m_clientNames.isEmpty()) return;

    // все возможных рисовальщики кроме последнего
    QSet<QString> possibleDrawers;
    for (const QString& name : m_clientNames.values()) {
        if (name != lastDrawer) {
            possibleDrawers.insert(name);
        }
    }

    if (possibleDrawers.isEmpty()) {
        m_currentDrawer = m_clientNames.values().at(QRandomGenerator::global()->bounded(m_clientNames.size()));
    } else {
        int randomIndex = QRandomGenerator::global()->bounded(possibleDrawers.size());
        auto it = possibleDrawers.begin();
        std::advance(it, randomIndex);
        m_currentDrawer = *it;
    }

    lastDrawer = m_currentDrawer;
}

QString myserver::selectRandomWord(){

    if (m_words.isEmpty()) return "";
    return m_words.at(QRandomGenerator::global()->bounded(m_words.size()));
}

void myserver::onRoundTimerTimeout(){

    endRound();
    ifOver();
}


void myserver::ifOver(){
    bool gameOver = true;
    int totalScore = 0;

    for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
        if (it.value() < 10) {
            gameOver = false;
            break;
        }
        totalScore += it.value();
    }

    if (gameOver && totalScore > 100) {
        gameOverLogic();
    }
}

void myserver::gameOverLogic(){
    QJsonObject gameOver;
    gameOver["type"] = "gameOver";

    QJsonObject scoresObject;
    for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
        scoresObject[it.key()] = it.value();
    }
    gameOver["scores"] = scoresObject;
    broadcast(gameOver);
}



void myserver::sendToClient(QTcpSocket *socket, const QJsonObject &message){

    if (!socket || socket->state() != QTcpSocket::ConnectedState) return;

    QJsonDocument doc(message);
    socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void myserver::broadcast(const QJsonObject& message, QTcpSocket *exclude){

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    for (QTcpSocket* socket : m_clients){
        if (socket != exclude && socket->state() == QTcpSocket::ConnectedState){
            socket->write(data);
        }
    }
}

//Для смены ролей from Kirya

void myserver::assignRoles() {
    if (clients.size() == 1) {
        clients[clients.keys().first()] = true; // Первый — художник
    } else {
        clients[clients.keys().last()] = false; // Остальные — наблюдатели
    }
}







































