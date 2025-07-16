#include "myserver.h"

myserver::myserver(QObject *parent) : QTcpServer(parent),
    m_gameState(WaitingForPlayers),
    m_currentRound(0)
{
    m_words = {"Крокодил", "Самолет", "Малыш Йода", "Яблоко", "Программист", "Слон"};
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

void myserver::processMessage(const QJsonObject &message, QTcpSocket *sender){

    QString type = message["type"].toString();

    if (type == "register"){

        QString name = message["name"].toString();
        m_clientNames[sender] = name;
        m_scores[name] = 0;
        QJsonObject response;
        response["type"] = "registered";
        response["sucsess"] = true;
        sendToClient(sender, response);

        QJsonObject playerJoined;
        playerJoined["type"] = "playerJoined";
        playerJoined["name"] = name;
        broadcast(playerJoined, sender);

        if (m_gameState == WaitingForPlayers && m_clientNames.size() >= 2){
            startGame();
        }
    } else if (type == "draw") {

        if (m_gameState == Drawing && m_clientNames[sender] == m_currentDrawer){
            broadcast(message, sender);
        }
    } else if (type == "guess") {
        // Обработка предположения
        if (m_gameState == Drawing && m_clientNames[sender] != m_currentDrawer) {
            QString guess = message["text"].toString();

            if (guess.compare(m_currentWord, Qt::CaseInsensitive) == 0) {
                // Правильный ответ
                QString guesser = m_clientNames[sender];
                m_scores[guesser] += 10;
                m_scores[m_currentDrawer] += 5;

                QJsonObject correctGuess;
                correctGuess["type"] = "correctGuess";
                correctGuess["guesser"] = guesser;
                correctGuess["word"] = m_currentWord;

                // Создаем QJsonObject с очками
                QJsonObject scoresObject;
                for (auto it = m_scores.begin(); it != m_scores.end(); ++it) {
                    scoresObject[it.key()] = it.value();
                }
                correctGuess["scores"] = scoresObject;

                broadcast(correctGuess);

                endRound();
            } else {
                // Неправильный ответ - пересылаем в чат
                QJsonObject chatMessage;
                chatMessage["type"] = "chat";
                chatMessage["player"] = m_clientNames[sender];
                chatMessage["text"] = guess;
                broadcast(chatMessage);
            }
        }
    }

    //
    qDebug() << "Server received:" << QJsonDocument(message).toJson(QJsonDocument::Compact);
}

void myserver::startGame(){
    m_gameState = Drawing;
    m_currentRound = 1;
    startNewRound();
}

void myserver::startNewRound() {

    selectNewDrawer();
    m_currentWord = selectRandomWord();

    QTcpSocket* drawerSocket = m_clientNames.key(m_currentDrawer);

    if (drawerSocket){

        QJsonObject drawerMessage;
        drawerMessage["type"] = "yourTurn";
        drawerMessage["word"] = m_currentWord;
        sendToClient(drawerSocket, drawerMessage);
    }

    QJsonObject roundStart;
    roundStart["type"] = "roundStart";
    roundStart["drawer"] = m_currentDrawer;
    roundStart["round"] = m_currentRound;
    broadcast(roundStart);

    m_roundTimer.start(60000);
}

void myserver::endRound() {
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

    QTimer::singleShot(5000, this, &myserver::startNewRound);
}

void myserver::selectNewDrawer() {

    if (m_clientNames.isEmpty()) return;

    int nextIndex = QRandomGenerator::global()->bounded(m_clientNames.size());
    m_currentDrawer = m_clientNames.values().at(nextIndex);
}

QString myserver::selectRandomWord(){

    if (m_words.isEmpty()) return "";
    return m_words.at(QRandomGenerator::global()->bounded(m_words.size()));
}

void myserver::onRoundTimerTimeout(){

    endRound();
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










































