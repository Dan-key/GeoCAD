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
    virtual void mousePressEvent(QMouseEvent *event, ViewportContext cntx)
    {
        auto modifiers = QGuiApplication::queryKeyboardModifiers();

        if ((event->buttons() & Qt::MiddleButton || (event->buttons() & Qt::RightButton && modifiers & Qt::ControlModifier))) {
            m_mousePressed = true;
            beginPos = event->position();
            deltaPos = VulkanRenderNode::pos;
        }
    };

    virtual void mouseMoveEvent(QMouseEvent *event, ViewportContext cntx)
    {
        if (m_mousePressed && (event->buttons() & Qt::MiddleButton || event->buttons() & Qt::RightButton)) {
            auto delta = event->position() - beginPos;
            _controller->updatePosition(deltaPos + delta);
        }
    };

    virtual void mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx)
    {
        m_mousePressed = false;

        beginPos = {0.0, 0.0};
        deltaPos = {0.0, 0.0};
    };
    virtual void hoverEnterEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void hoverMoveEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void hoverLeaveEvent(QHoverEvent *event, ViewportContext cntx) {};
    virtual void wheelEvent(QWheelEvent *event, ViewportContext cntx) {};
    virtual void keyPressEvent(QKeyEvent* event, ViewportContext cntx) {};
    virtual ~IModeHandler() = default;
protected:
    MainWindow* _controller = nullptr;
    bool m_mousePressed = false;
    QPointF deltaPos;
    QPointF beginPos;
};

} // namespace ModeHandlers