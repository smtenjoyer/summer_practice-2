#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QMessageBox>
#include <QRegularExpression>
#include <QHostAddress>
#include "gamewindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void sendJsonMessage(const QJsonObject &message); // Keep this function, needed to initialize the connection

private slots:
    void on_connectButton_clicked();
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    Ui::MainWindow *ui;
    QTcpSocket* m_socket;
    QString m_playerName;
    GameWindow* m_gameWindow = nullptr; // Initialize to nullptr
};

#endif // MAINWINDOW_H
