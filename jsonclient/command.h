#ifndef COMMAND_H
#define COMMAND_H

#include <QUndoCommand>
#include <QPointF>
#include <QColor>
#include <QImage>
#include "doodlearea.h"
#include <QPainter>
#include <QUndoCommand>

class DrawLineCommand : public QUndoCommand {
public:
    DrawLineCommand(DoodleArea *doodleArea, const QPoint &lastPoint, const QPoint &endPoint, DoodleArea::ShapeType tool, const QColor &penColor, int penWidth, const QImage& oldImage, const QImage& newImage)
        : doodleArea(doodleArea), lastPoint(lastPoint), endPoint(endPoint), tool(tool), penColor(penColor), penWidth(penWidth), oldImage(oldImage), newImage(newImage) {
        setText(QObject::tr("Draw Line")); // User-friendly name in Undo/Redo menu
    }

    void undo() override {
        doodleArea->setImage(oldImage);
        doodleArea->update();
    }

    void redo() override {
        doodleArea->setImage(newImage);
        doodleArea->update();
    }

private:
    DoodleArea *doodleArea;
    QPoint lastPoint;
    QPoint endPoint;
    DoodleArea::ShapeType tool;
    QColor penColor;
    int penWidth;
    QImage oldImage;
    QImage newImage;
};

// Command to fill an area
class FillAreaCommand : public QUndoCommand {
public:
    FillAreaCommand(DoodleArea *doodleArea, const QPoint &seedPoint, const QColor &penColor, const QImage& oldImage, const QImage& newImage)
        : doodleArea(doodleArea), seedPoint(seedPoint), penColor(penColor), oldImage(oldImage), newImage(newImage) {
        setText(QObject::tr("Fill Area"));
    }

    void undo() override {
        doodleArea->setImage(oldImage);
        doodleArea->update();
    }

    void redo() override {
        doodleArea->setImage(newImage);
        doodleArea->update();
    }

private:
    DoodleArea *doodleArea;
    QPoint seedPoint;
    QColor penColor;
    QImage oldImage;
    QImage newImage;
};

// Command to draw a shape (Line, Rectangle, Ellipse)
class DrawShapeCommand : public QUndoCommand {
public:
    DrawShapeCommand(DoodleArea *doodleArea, const QPoint &lastPoint, const QPoint &endPoint, DoodleArea::ShapeType tool, const QColor &penColor, int penWidth, const QImage& oldImage, const QImage& newImage)
        : doodleArea(doodleArea), lastPoint(lastPoint), endPoint(endPoint), tool(tool), penColor(penColor), penWidth(penWidth), oldImage(oldImage), newImage(newImage) {
        setText(QObject::tr("Draw Shape"));
    }

    void undo() override {
        doodleArea->setImage(oldImage);
        doodleArea->update();
    }

    void redo() override {
        doodleArea->setImage(newImage);
        doodleArea->update();
    }

private:
    DoodleArea *doodleArea;
    QPoint lastPoint;
    QPoint endPoint;
    DoodleArea::ShapeType tool;
    QColor penColor;
    int penWidth;
    QImage oldImage;
    QImage newImage;
};

#endif // COMMAND_H


