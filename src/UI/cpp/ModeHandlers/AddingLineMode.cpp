#include "AddingLineMode.h"
#include "UI/cpp/MainWindow.h"
#include "UI/cpp/Geometry/Geometry.h"
#include <QObject>
#include "../VulkanRenderNode.h"

namespace ModeHandlers {

AddingLineMode::AddingLineMode(MainWindow* controller)
    : IModeHandler(controller)
{}

void AddingLineMode::mousePressEvent(QMouseEvent *event, ViewportContext cntx)
{
    QPointF localPos = event->position();

    if (event->buttons() & Qt::RightButton) {
        _controller->changeMode(MainWindow::Mode::None);
        isSecondPoint = false;
        QGuiApplication::restoreOverrideCursor();
        return;
    } else if (event->buttons() & Qt::LeftButton) {
        m_mouseLinePressed = true;
    }

    if (!isSecondPoint) {
        auto itemSize = cntx.viewportSize;
        auto pos = cntx.offset;
        auto z = cntx.zoomLevel;

        addLineStart = QPointF(
            (((event->position().x() - pos.x() / 2) * 2 / (itemSize.width()) - 1) / z),
            (((event->position().y() - pos.y() / 2) * 2 / (itemSize.height()) - 1) / z)
        );
        qDebug() << "addLineStart" << addLineStart << "eventposition" << event->position();
        // isSecondPoint = true;
    }
    IModeHandler::mousePressEvent(event, cntx);
}

void AddingLineMode::mouseMoveEvent(QMouseEvent *event, ViewportContext cntx)
{
    QPointF localPos = event->position();

    auto itemSize = cntx.viewportSize;
    auto pos = cntx.offset;
    auto z = cntx.zoomLevel;

    float endXpos = ((localPos.x() - pos.x() / 2) * 2 / itemSize.width() - 1) / z;
    float endYpos = ((localPos.y() - pos.y() / 2) * 2 / itemSize.height() - 1) / z;
    Geometry::Line line = {
        Geometry::Vertex{(float)addLineStart.x(), (float)addLineStart.y(), 0., 0., 0.}, 
        Geometry::Vertex{endXpos, endYpos, 0., 0., 0.}
    };
    if (m_mouseLinePressed) {
        if (isSecondPoint) {
            _controller->updateLine(line);
        } else {
            isSecondPoint = true;
            _controller->addLine(line);
        }
    }
    IModeHandler::mouseMoveEvent(event, cntx);
}

void AddingLineMode::mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx)
{
    m_mouseLinePressed = false;
    QPointF localPos = event->position();

    beginPos = {0.0, 0.0};
    deltaPos = {0.0, 0.0};

    if (isSecondPoint) {
        auto itemSize = cntx.viewportSize;
        auto pos = cntx.offset;
        auto z = cntx.zoomLevel;

        float endXpos = ((event->position().x() -pos.x() / 2) * 2 / itemSize.width() - 1) / z;
        float endYpos = ((event->position().y() - pos.y() / 2) * 2 / itemSize.height() - 1) / z;

        Geometry::Line line = {
            Geometry::Vertex{(float)addLineStart.x(), (float)(addLineStart.y()),0.0, 0.0, 0.0},
            Geometry::Vertex{endXpos, endYpos, 0.0, 0.0, 0.0}
        };
        _controller->addLine(line);
        isSecondPoint = false;
        _controller->changeMode(MainWindow::Mode::None);
        QGuiApplication::restoreOverrideCursor();

        return;
    } else {
        isSecondPoint = true;
    }
    IModeHandler::mouseReleaseEvent(event, cntx);
}

} // namespace ModeHandlers
