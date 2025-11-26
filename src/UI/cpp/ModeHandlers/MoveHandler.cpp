#include "MoveHandler.h"
#include "../MainWindow.h"

namespace ModeHandlers {

MoveHandler::MoveHandler(MainWindow *controller) : IModeHandler(controller) {};

void MoveHandler::mousePressEvent(QMouseEvent *event, ViewportContext cntx)
{
    auto modifiers = QGuiApplication::queryKeyboardModifiers();

    if ((event->buttons() & Qt::MiddleButton || (event->buttons() & Qt::RightButton && modifiers & Qt::ControlModifier))) {
        m_mousePressed = true;
        beginPos = event->position();
        deltaPos = VulkanRenderNode::pos;
    }
};

void MoveHandler::mouseMoveEvent(QMouseEvent *event, ViewportContext cntx)
{
    if (m_mousePressed && (event->buttons() & Qt::MiddleButton || event->buttons() & Qt::RightButton)) {
        auto delta = event->position() - beginPos;
        _controller->updatePosition(deltaPos + delta);
    }
};

void MoveHandler::mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx)
{
    m_mousePressed = false;

    beginPos = {0.0, 0.0};
    deltaPos = {0.0, 0.0};
};

void MoveHandler::wheelEvent(QWheelEvent* event, ViewportContext cntx)
{
    float delta = event->angleDelta().ry();
    if (delta > 0) {
        VulkanRenderNode::z /= 0.9;
    } else if (delta < 0 && VulkanRenderNode::z > 0.04) {
        VulkanRenderNode::z *= 0.9;
    }
}

}