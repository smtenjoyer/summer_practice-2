#include <QtWidgets>
#include "doodlearea.h"
#include "command.h"

DoodleArea::DoodleArea(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StaticContents);
    doodling = false;
    myPenWidth = 1;
    myPenColor = Qt::blue;
    currentTool = Pencil;
    undoStack = new QUndoStack(this);
    setMouseTracking(true);
    textInputStartPoint = QPoint(0, 0);
    textFont = QFont("Arial", 12);
    textColor = Qt::black;
    //работает Киря не прокосаться
    lastRemotePoint = QPoint(0, 0);
    remotePenColor = Qt::black;
    remotePenWidth = 1;
    remoteTool = Pencil;
    //
}

DoodleArea::DoodleArea(const QSize& size, QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StaticContents);
    doodling = false;
    myPenWidth = 1;
    myPenColor = Qt::blue;
    currentTool = Pencil;
    undoStack = new QUndoStack(this);
    setMouseTracking(true);

    image = QImage(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    setFixedSize(size);
    textInputStartPoint = QPoint(0, 0);
    textFont = QFont("Arial", 12);
    textColor = Qt::black;
    //работает Киря не прокосаться
    lastRemotePoint = QPoint(0, 0);
    remotePenColor = Qt::black;
    remotePenWidth = 1;
    remoteTool = Pencil;
    //

}

void DoodleArea::setScaleFactor(double scaleFactor) {
    m_scaleFactor = scaleFactor;
    update();
}

bool DoodleArea::ifModified(){
    return modified;
}

bool DoodleArea::openImage(const QString &fileName){
    QImage loadedImage;
    if(!loadedImage.load(fileName)){
        return false;
    }
    QSize newSize = loadedImage.size().expandedTo(size());
    setFixedSize(newSize);
    image = loadedImage;
    modified = false;
    update();
    return true;
}

bool DoodleArea::saveImage(const QString &fileName, const char *fileFormat){
    QImage visibleImage = image;
    resizeImage(&visibleImage, size());
    if(visibleImage.save(fileName, fileFormat)){
        modified = false;
        return true;
    } else {
        return false;
    }
}

void DoodleArea::setPenColor(const QColor &newColor){
    myPenColor = newColor;
}

void DoodleArea::setPenWidth(int newWidth){
    myPenWidth = newWidth;
}

void DoodleArea::clearImage(){
    image.fill(QColor(255,255,255));
    modified = true;
    update();
}


void DoodleArea::mousePressEvent(QMouseEvent *event) {
    if(event->button()==Qt::LeftButton) {
        if (currentTool == Fill) {
            oldImage = image.copy();
            fillArea(event->pos());

            DrawShapeCommand *fillCommand = new DrawShapeCommand(this, event->pos(), event->pos(),
                                                                 currentTool, myPenColor, myPenWidth, oldImage, image.copy());
            undoStack->push(fillCommand);

            QJsonObject cmd;
            cmd["type"] = "draw";
            cmd["tool"] = "fill";
            cmd["x"] = event->pos().x();
            cmd["y"] = event->pos().y();
            cmd["color"] = myPenColor.name();
            emit drawingCommandGenerated(cmd);
        }
        else if (currentTool == Pencil || currentTool == Rubber ||
                 currentTool == Rectangle || currentTool == Ellipse ||
                 currentTool == Line) {
            lastPoint = event->pos();
            doodling = true;
            oldImage = image.copy();

            QJsonObject cmd;
            cmd["type"] = "draw";
            cmd["tool"] = (currentTool == Pencil) ? "pencil" :
                              (currentTool == Rubber) ? "rubber" :
                              (currentTool == Rectangle) ? "rectangle" :
                              (currentTool == Ellipse) ? "ellipse" : "line";
            cmd["action"] = "start";
            cmd["x"] = event->pos().x();
            cmd["y"] = event->pos().y();
            cmd["color"] = myPenColor.name();
            cmd["width"] = myPenWidth;
            emit drawingCommandGenerated(cmd);
        }
        else if (currentTool == Textt){
            if (isTextInputActive) {
                finishTextInput();
            }

            textInputStartPoint = event->pos();
            isTextInputActive = true;

            textInput = new QLineEdit(this);
            textInput->move(textInputStartPoint);
            textInput->setFont(textFont);
            textInput->setStyleSheet("QLineEdit { background-color: white; color: black; border: 1px solid black; }");
            textInput->show();
            textInput->setFocus();

            connect(textInput, &QLineEdit::editingFinished, this, &DoodleArea::finishTextInput);


            textInput->installEventFilter(this);
        }
    }
}

void DoodleArea::finishTextInput() {
    if (textInput) {
        QString text = textInput->text();

        QPainter painter(&image);
        painter.setFont(textFont);
        painter.setPen(textColor);
        painter.drawText(textInputStartPoint, text);
        painter.end();

        textInput->deleteLater();
        textInput = nullptr;
        isTextInputActive = false;
        update();
    }
}

/*void DoodleArea::mouseMoveEvent(QMouseEvent *event) {
    static int counter = 0;
    const int SAMPLE_RATE = 3; // Отправлять каждую 3-ю точку
    if (doodling && (currentTool == Pencil || currentTool == Rubber)) {
        counter++;
        if (counter % SAMPLE_RATE == 0) {
            QJsonObject cmd;
            cmd["type"] = "draw_point";
            cmd["x"] = event->pos().x();
            cmd["y"] = event->pos().y();
            emit drawingCommandGenerated(cmd);
        }

    if (doodling) {
        QPoint endPoint = event->pos();
        if (currentTool == Pencil || currentTool == Rubber) {
            drawLineTo(endPoint);
        } else {
            tempImage = image.copy();
            drawShape(endPoint, &tempImage);
            update();
        }
    }
    }
}*/
void DoodleArea::mouseMoveEvent(QMouseEvent *event) {
    if (doodling) {
        QPoint endPoint = event->pos();
        if (currentTool == Pencil || currentTool == Rubber) {
            drawLineTo(endPoint);

            QJsonObject cmd;
            cmd["type"] = "draw";
            cmd["tool"] = (currentTool == Pencil) ? "pencil" : "rubber";
            cmd["action"] = "move";
            cmd["x1"] = lastPoint.x();
            cmd["y1"] = lastPoint.y();
            cmd["x2"] = endPoint.x();
            cmd["y2"] = endPoint.y();
            cmd["color"] = myPenColor.name();
            cmd["width"] = myPenWidth;
            emit drawingCommandGenerated(cmd);

            lastPoint = endPoint;
        } else {
            tempImage = image.copy();
            drawShape(endPoint, &tempImage);
            update();
        }
    }
}

/*void DoodleArea::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && doodling) {
        QPoint endPoint = event->pos();
        QImage finalImage = image.copy();

        if (currentTool == Pencil || currentTool == Rubber) {
            drawLineTo(endPoint);
            finalImage = image.copy();
        }
        else{
            drawShape(endPoint, &finalImage);
        }
        DrawShapeCommand *command = new DrawShapeCommand(this, lastPoint, endPoint, currentTool, myPenColor, myPenWidth, oldImage, finalImage);
        undoStack->push(command);

        doodling = false;
        tempImage = QImage();
    }
}*/
void DoodleArea::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && doodling) {
        QPoint endPoint = event->pos();
        QImage finalImage = image.copy();

        if (currentTool == Pencil || currentTool == Rubber) {
            drawLineTo(endPoint);
            finalImage = image.copy();
        } else {
            drawShape(endPoint, &finalImage);

            QJsonObject cmd;
            cmd["type"] = "draw";
            cmd["tool"] = (currentTool == Rectangle) ? "rectangle" :
                              (currentTool == Ellipse) ? "ellipse" : "line";
            cmd["x1"] = lastPoint.x();
            cmd["y1"] = lastPoint.y();
            cmd["x2"] = endPoint.x();
            cmd["y2"] = endPoint.y();
            cmd["color"] = myPenColor.name();
            cmd["width"] = myPenWidth;
            emit drawingCommandGenerated(cmd);
        }

        DrawShapeCommand *command = new DrawShapeCommand(this, lastPoint, endPoint,
                                                         currentTool, myPenColor, myPenWidth, oldImage, finalImage);
        undoStack->push(command);

        doodling = false;
        tempImage = QImage();
    }
}


void DoodleArea::setTool(ShapeType tool) {
    currentTool = tool;
    doodling = false;
}

void DoodleArea::undo() {
    undoStack->undo();
}

void DoodleArea::redo() {
    undoStack->redo();
}

void DoodleArea::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true); // Для более гладкой отрисовки

    painter.save();

    painter.translate(m_offset);
    painter.scale(m_scaleFactor, m_scaleFactor);

    if (doodling && currentTool != Pencil && currentTool != Rubber) {
        painter.drawImage(0, 0, tempImage);
    } else {
        painter.drawImage(0, 0, image);
    }

    painter.restore();

    QPen framePen(Qt::black, 1, Qt::SolidLine);
    painter.setPen(framePen);
    painter.setBrush(Qt::NoBrush);

    QRect imageRect(m_offset.x(), m_offset.y(), image.width() * m_scaleFactor, image.height() * m_scaleFactor);
    painter.drawRect(imageRect);
}

void DoodleArea::resizeEvent(QResizeEvent *event) {
    if (event->size().width() > image.width() || event->size().height() > image.height()) {
        QImage newImage(event->size(), QImage::Format_ARGB32_Premultiplied);
        newImage.fill(QColor(255,255,255));

        QPainter painter(&newImage);
        painter.drawImage(QPoint(0, 0), image);
        image = newImage;
    }
    QWidget::resizeEvent(event);
}

void DoodleArea::drawLineTo(const QPoint &endPoint){

    QPainter painter(&image);
    if (currentTool == Pencil){
        painter.setPen(QPen(myPenColor, myPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else if (currentTool == Rubber){
        painter.setPen(QPen(QColor(255,255,255), myPenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    painter.drawLine(lastPoint, endPoint);
    modified = true;
    int rad = (myPenWidth / 2) + 2;
    update(QRect(lastPoint, endPoint).normalized().adjusted(-rad, -rad, +rad, +rad));
    //Работает Киря не прокасаться
    QJsonObject cmd;
    cmd["type"] = "draw_line";
    cmd["x1"] = lastPoint.x();
    cmd["y1"] = lastPoint.y();
    cmd["x2"] = endPoint.x();
    cmd["y2"] = endPoint.y();
    emit drawingCommandGenerated(cmd);
    //
    lastPoint = endPoint;
}

QImage DoodleArea::getImage() const {
    return image;
}

void DoodleArea::setImage(const QImage &newImage) {
    image = newImage;
}

void DoodleArea::resizeImage(QImage *image, const QSize &newSize){
    if(image->size() == newSize){
        return;
    }

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0,0), *image);
    *image = newImage;
}

void DoodleArea::resizeCanvas() {
    QSize currentSize = image.size();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Новый размер изображения"));

    QFormLayout form(&dialog);

    QLineEdit *widthEdit = new QLineEdit(&dialog);
    widthEdit->setValidator(new QIntValidator(1, 2000, this));
    widthEdit->setText("1920");
    form.addRow(tr("Ширина:"), widthEdit);

    QLineEdit *heightEdit = new QLineEdit(&dialog);
    heightEdit->setValidator(new QIntValidator(1, 2000, this));
    heightEdit->setText("1080");
    form.addRow(tr("Высота:"), heightEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted) {
        bool okWidth, okHeight;
        int newWidth = widthEdit->text().toInt(&okWidth);
        int newHeight = heightEdit->text().toInt(&okHeight);

        if (okWidth && okHeight) {
            QSize newSize(newWidth, newHeight);

            QImage newImage(newSize, QImage::Format_ARGB32_Premultiplied);
            newImage.fill(Qt::white);

            QPainter painter(&newImage);

            QRect copyRect(QPoint(0, 0), QSize(qMin(currentSize.width(), newWidth),
                                               qMin(currentSize.height(), newHeight)));

            painter.drawImage(QPoint(0, 0), image, copyRect);

            image = newImage;


            update();
            setFixedSize(newSize);
            modified = true;
        } else {
            QMessageBox::warning(this, tr("Ошибка"), tr("Пожалуйста, введите корректные числа от 1 до 2000."));
        }
    }
}

void DoodleArea::fillArea(const QPoint &seedPoint) {
    if (!image.valid(seedPoint)) {
        qDebug() << "Seed point is invalid!";
        return;
    }

    QColor targetColor = image.pixelColor(seedPoint);
    if (targetColor == myPenColor) {
        // Область уже залита этим цветом, ничего не делаем
        return;
    }

    QQueue<QPoint> pointsToFill; // Очередь точек для заливки
    pointsToFill.enqueue(seedPoint); // Добавляем начальную точку в очередь

    QImage newImage = image; // Создаем копию image для работы

    while (!pointsToFill.isEmpty()) {
        QPoint currentPoint = pointsToFill.dequeue();
        int x = currentPoint.x();
        int y = currentPoint.y();

        // Проверка границ изображения уже выполняется image.valid(seedPoint)
        if (!newImage.valid(currentPoint)) {
            continue;
        }


        // Проверка, что текущая точка имеет целевой цвет
        if (newImage.pixelColor(x, y) == targetColor) {
            newImage.setPixelColor(x, y, myPenColor); // Заливаем пиксель новым цветом
            // Добавляем соседние пиксели в очередь
            pointsToFill.enqueue(QPoint(x + 1, y));
            pointsToFill.enqueue(QPoint(x - 1, y));
            pointsToFill.enqueue(QPoint(x, y + 1));
            pointsToFill.enqueue(QPoint(x, y - 1));
        }
    }

    image = newImage; // Заменяем исходное изображение новой версией

    modified = true;
    update();
}

void DoodleArea::drawShape(const QPoint &endPoint, QImage *targetImage) {
    QPainter painter(targetImage);
    painter.setPen(QPen(myPenColor, myPenWidth, Qt::SolidLine));

    switch (currentTool) {
    case Line:
        painter.drawLine(lastPoint, endPoint);
        modified = true;
        break;
    case Rectangle:
        painter.drawRect(QRect(lastPoint, endPoint).normalized());
        modified = true;
        break;
    case Ellipse:
        painter.drawEllipse(QRect(lastPoint, endPoint).normalized());
        modified = true;
        break;
    default:
        break;
    }
}

QUndoStack* DoodleArea::getUndoStack() const {
    return undoStack;
}
//Работае Киря не прикосаться
// Новая функция для применения удаленных команд
void DoodleArea::applyRemoteCommand(const QJsonObject &command) {
    QString tool = command["tool"].toString();

    if (tool == "clear") {
        image.fill(Qt::white);
        update();
        return;
    }

    QPainter painter(&image);
    QColor color = QColor(command["color"].toString());
    int width = command["width"].toInt();

    if (tool == "pencil" || tool == "rubber") {
        if (tool == "rubber") color = Qt::white;

        painter.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        if (command.contains("x1")) {
            // Линия (движение)
            QPoint p1(command["x1"].toInt(), command["y1"].toInt());
            QPoint p2(command["x2"].toInt(), command["y2"].toInt());
            painter.drawLine(p1, p2);
        } else {
            // Точка (начало)
            QPoint p(command["x"].toInt(), command["y"].toInt());
            painter.drawPoint(p);
        }
    }
    else if (tool == "fill") {
        QColor origColor = myPenColor;
        myPenColor = color;
        fillArea(QPoint(command["x"].toInt(), command["y"].toInt()));
        myPenColor = origColor;
    }
    else if (tool == "rectangle") {
        painter.setPen(QPen(color, width));
        painter.drawRect(QRect(
            QPoint(command["x1"].toInt(), command["y1"].toInt()),
            QPoint(command["x2"].toInt(), command["y2"].toInt())
        ));
    }
    else if (tool == "ellipse") {
        painter.setPen(QPen(color, width));
        painter.drawEllipse(QRect(
            QPoint(command["x1"].toInt(), command["y1"].toInt()),
            QPoint(command["x2"].toInt(), command["y2"].toInt())
            ));
    }
    else if (tool == "line") {
        painter.setPen(QPen(color, width));
        painter.drawLine(
            QPoint(command["x1"].toInt(), command["y1"].toInt()),
            QPoint(command["x2"].toInt(), command["y2"].toInt())
        );
    }

    update();
}
void DoodleArea::setupRemotePainter(QPainter &painter) {
    if (remoteTool == Rubber) {
        painter.setPen(QPen(Qt::white, remotePenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    } else {
        painter.setPen(QPen(remotePenColor, remotePenWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
}
