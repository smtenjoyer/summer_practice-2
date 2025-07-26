#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPoint>
#include <QToolBar>
#include <QColorDialog>
#include <QActionGroup>
#include <QLabel>
#include <QPainter>

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

    void penColor();
    void penWidth();

    void updateBrushPreview(QLabel *label, int width, QColor color);


    //Киря
public slots:
    void processServerMessage(const QJsonObject &message);
    void updateScoresTable(const QJsonObject& scores);



private:
    //
    void updateAllPlayersTable(const QJsonObject& scores);
    //
    Ui::GameWindow *ui;
    QTcpSocket* m_socket;
    QString m_playerName;
    bool m_isDrawing;

    DoodleArea *m_doodleArea = nullptr;


    QAction *penColorAct;
    QAction *penWidthAct;
    QAction *clearScreenAct;
    QAction *fillAreaAct;
    QAction *PencilAct;
    QAction *RubberAct;
    QAction *lineAction;
    QAction *rectangleAction;
    QAction *ellipseAction;

    QAction *undoActionBtn;
    QAction *redoActionBtn;


    void createActions();
    void createToolBars();

    void setActions(const bool& drawing);


};

#endif // GAMEWINDOW_H
