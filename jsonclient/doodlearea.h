#ifndef DOODLEAREA_H
#define DOODLEAREA_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <QUndoStack>
#include <QScrollBar>
#include <QGraphicsPixmapItem>
#include <QLineEdit>


class DoodleArea : public QWidget
{
    Q_OBJECT

public:
    enum ShapeType {
        Pencil,
        Rubber,
        Fill,
        Line,
        Rectangle,
        Ellipse,
        Textt
    };

public:
    DoodleArea(QWidget *parent = 0);
    DoodleArea(const QSize& size, QWidget *parent = nullptr);
    bool openImage(const QString &filename);
    bool saveImage(const QString &filename, const char *fileFormat);
    void setPenColor(const QColor &newColor);
    void setPenWidth(int newWidth);
    bool isModified() const {return modified;}
    QColor penColor() const {return myPenColor;}
    int penWidth() const {return myPenWidth;}
    QUndoStack* getUndoStack() const;
    void setTool(ShapeType tool);
    QImage getImage() const;
    void setImage(const QImage &newImage);

    bool ifModified();
    void setScaleFactor(double scaleFactor);
    double scaleFactor() const { return m_scaleFactor; }
    void drawLineTo(const QPoint &endPoint);
    void drawRemotePoints(const QVector<QPoint> &points);
    int myPenWidth;
signals:
    void drawingPointsChanged(const QVector<QPoint> &points);
public slots:
    void clearImage();
    void resizeCanvas();

    void undo();
    void redo();

private:
    void mousePressEvent(QMouseEvent *event) override;
    void finishTextInput();

    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void setImageItem(QGraphicsPixmapItem *item);
    QVector<QPoint> currentStrokePoints;

    void drawShape(const QPoint &endPoint, QImage *targetImage);
    void resizeImage(QImage *image, const QSize &newSize);

    void fillArea(const QPoint &seedPoint);

    bool modified = false;
    bool doodling;
    bool shaping;

    QColor myPenColor;


    QImage oldImage;
    QImage tempImage;

    QImage image;
    QPoint lastPoint;
    ShapeType currentTool;
    QPoint textInputStartPoint;
    QLineEdit *textInput = nullptr;
    bool isTextInputActive = false;
    QFont textFont;
    QColor textColor;


    QUndoStack *undoStack;
    QGraphicsPixmapItem *imageItem = nullptr;

    double m_scaleFactor = 1.0;
    QPoint m_offset;
    QPoint m_lastMousePosition;
};

#endif // DOODLEAREA_H
