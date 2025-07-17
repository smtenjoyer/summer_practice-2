#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPoint>

class DoodleArea;

namespace Ui {
class GameWindow;
}

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(QTcpSocket* socket, const QString& playerName, QWidget *parent = nullptr);
    ~GameWindow() override;

    void processServerMessage(const QJsonObject &message);

    void setupGameUI(bool isDrawer);

signals:
    void sendMessage(const QJsonObject& message);
    //работает Киря не прокосаться
    void sendDrawingCommand(const QJsonObject& command);
    //
private slots:
    void on_sendGuessButton_clicked();
    void sendDrawingPoints(const QVector<QPoint>& points);


private:
    Ui::GameWindow *ui;
    QTcpSocket* m_socket;
    QString m_playerName;
    bool m_isDrawing;

    DoodleArea *m_doodleArea = nullptr;

};

#endif // GAMEWINDOW_H
