#pragma once

#include <QPointF>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QWheelEvent>
#include "UI/cpp/MainWindow.h"
#include "ViewportContext.h"
#include "UI/cpp/VulkanRenderNode.h"

class MainWindow;

namespace ModeHandlers {

class IModeHandler
{
public:
    IModeHandler(MainWindow* controller) : _controller(controller) {};
    virtual void mousePressEvent(QMouseEvent *event, ViewportContext cntx) {};
    virtual void mouseMoveEvent(QMouseEvent *event, ViewportContext cntx) {};
    virtual void mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx) {};
    virtual void hoverEnterEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void hoverMoveEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void hoverLeaveEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void wheelEvent(QWheelEvent *event, ViewportContext cntx) {};
    virtual void keyPressEvent(QKeyEvent* event, ViewportContext cntx) {};
    virtual ~IModeHandler() = default;
protected:
    MainWindow* _controller = nullptr;
};

} // namespace ModeHandlers