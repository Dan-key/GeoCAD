#pragma once

#include <iostream>
#include <QObject>
#include <QSizeF>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include "Library/Flux/MutableList.h"
#include "ModeHandlers/ViewportContext.h"
#include <linux/limits.h>
#include <memory>

#include "Library/Meta/Meta.h"
#include "Geometry/Line.h"

namespace ModeHandlers {
    class IModeHandler;
}

class MainWindow : public QObject
{
    Q_OBJECT
public:
    MainWindow(QObject* parent = nullptr);

    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;

    EXPOSE(x1);
    EXPOSE(y1);
    EXPOSE(x2);
    EXPOSE(y2);

    float angle = 0.0;
    EXPOSE(angle);

    enum class Mode {
        None,
        AddLine,
    };

    Mode currentMode = Mode::None;

    void changeMode(Mode newMode);
    QSizeF vulkanItemSize();

    void addLine(const Geometry::Line& line);
    void updateLine(const Geometry::Line& line);
    void updatePosition(const QPointF& position);

    Flux::MutableList<Geometry::Line> lines;

public slots:
    void mousePress(QMouseEvent* event, ViewportContext cntx);
    void mouseMove(QMouseEvent* event, ViewportContext cntx);
    void mouseRelease(QMouseEvent* event, ViewportContext cntx);
    void hoverEnter(QHoverEvent* event, ViewportContext cntx);
    void hoverMove(QHoverEvent* event, ViewportContext cntx);
    void hoverLeave(QHoverEvent* event, ViewportContext cntx);
    void wheel(QWheelEvent* event, ViewportContext cntx);
    void keyPress(QKeyEvent* event, ViewportContext cntx);

    void addLineMode();
    void addingLineWithCoordinates(float x1, float y1, float x2, float y2);

private:
    std::shared_ptr<ModeHandlers::IModeHandler> _modeController;
};