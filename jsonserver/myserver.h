#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QList>
#include <QString>
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>

class myserver: public QTcpServer
{
    Q_OBJECT
public:

    explicit myserver(QObject *parent = nullptr);
    ~myserver();

    enum GameState {
        WaitingForPlayers,
        Drawing,
        RoundEnd,
        GameEnd
    };

    void startServer();

private:

    // cокеты и данные
    QList<QTcpSocket*> m_clients;
    QMap<QTcpSocket*, QString> m_clientNames;
    QMap<QString, int> m_scores;
    QByteArray m_data;

    // bгровые переменные
    GameState m_gameState;
    int m_currentRound;
    QString m_currentWord;
    QString m_currentDrawer;
    QTimer m_roundTimer;
    QStringList m_words;

    //для смены ролей
    QMap<QTcpSocket*, bool> clients;  // true = художник, false = наблюдатель
    QList<QJsonObject> m_drawingHistory;
    bool m_isRoundActive; // Флаг активности раунда

    void assignRoles();
    // cетевые методы
    void sendToClient(QTcpSocket* socket, const QJsonObject& message);
    void broadcast(const QJsonObject& message, QTcpSocket* exclude = nullptr);
    void processMessage(const QJsonObject& message, QTcpSocket* sender);

    // bгровые методы
    void startGame();
    void startNewRound();
    void endRound();
    void selectNewDrawer();
    QString selectRandomWord();
    void updateAllClientsGameState();
//
signals:
    void roundStarted(const QString &drawerName);
//
protected:
    void incomingConnection(qintptr socketDescriptor) override;


private slots:
    void onReadyRead();
    void onDisconnected();
    void onRoundTimerTimeout();
};

#endif // MYSERVER_H
