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
    enum ShapeType {
        None,
        Pencil,
        Rubber,
        Fill,
        Line,
        Rectangle,
        Ellipse,
        Textt
    };

    explicit GameWindow(QTcpSocket* socket, const QString& playerName, QWidget *parent = nullptr);
    ~GameWindow() override;



    void setupGameUI(bool isDrawer);

signals:
    void sendMessage(const QJsonObject& message);
    //работает Киря не прокосаться
    void sendDrawingCommand(const QJsonObject& command);
    //
private slots:
    void processGameOver(const QJsonObject& scores);

    void on_sendGuessButton_clicked();
    void sendDrawingPoints(const QVector<QPoint>& points);

    void setFillTool();
    void setPencilTool();
    void setRubberTool();

    void setNoneTool();
    void setLineTool();
    void setRectangleTool();
    void setEllipseTool();
    void undoAction();
    void redoAction();

    //Киря
public slots:
    void processServerMessage(const QJsonObject &message);
    void updateScoresTable(const QJsonObject& scores);



private:
    Ui::GameWindow *ui;
    QTcpSocket* m_socket;
    QString m_playerName;
    bool m_isDrawing;

    DoodleArea *m_doodleArea = nullptr;

};

#endif // GAMEWINDOW_H
