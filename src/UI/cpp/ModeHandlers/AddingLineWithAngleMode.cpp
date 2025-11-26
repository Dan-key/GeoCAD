#include "AddingLineWithAngleMode.h"
#include "UI/cpp/Geometry/Line.h"
#include "UI/cpp/ModeHandlers/IModeHandler.h"
#include <QPoint>
#include <cmath>

namespace ModeHandlers {

AddingLineWithAngleMode::AddingLineWithAngleMode(MainWindow* controller)
    : IModeHandler(controller)
{
}

void AddingLineWithAngleMode::mousePressEvent(QMouseEvent *event, ViewportContext cntx)
{
    QPointF localPos = event->position();

    if (event->buttons() & Qt::RightButton) {
        _controller->changeMode(MainWindow::Mode::None);
        QGuiApplication::restoreOverrideCursor();
        return;
    } else if (event->buttons() & Qt::LeftButton) {
        _pressed = true;
    }

    auto itemSize = cntx.viewportSize;
    auto pos = cntx.offset;
    auto z = cntx.zoomLevel;

    QPointF addLineStart = QPointF(
        (((event->position().x() - pos.x() / 2) * 2 / (itemSize.width()) - 1) / z),
        (((event->position().y() - pos.y() / 2) * 2 / (itemSize.height()) - 1) / z)
    );

    QPointF addLineEnd = QPointF(addLineStart.rx() + 10 * std::sin((_controller->angle + 90) / 180 * M_PI), addLineStart.ry() + 10 * std::cos((_controller->angle + 90) / 180 * M_PI));

    Geometry::Line line = {
        Geometry::Vertex{(float)addLineStart.x(), (float)addLineStart.y(), 0., 0., 0.}, 
        Geometry::Vertex{(float)addLineEnd.x(), (float)addLineEnd.y(), 0., 0., 0.}
    };

    _controller->addLine(line);
}

void AddingLineWithAngleMode::mouseMoveEvent(QMouseEvent *event, ViewportContext cntx)
{
    if (_pressed) {
        auto itemSize = cntx.viewportSize;
        auto pos = cntx.offset;
        auto z = cntx.zoomLevel;

        QPointF addLineStart = QPointF(
            (((event->position().x() - pos.x() / 2) * 2 / (itemSize.width()) - 1) / z),
            (((event->position().y() - pos.y() / 2) * 2 / (itemSize.height()) - 1) / z)
        );

        QPointF addLineEnd = QPointF(addLineStart.rx() + 10 * std::sin((_controller->angle + 90) / 180 * M_PI), addLineStart.ry() + 10 * std::cos((_controller->angle + 90) / 180 * M_PI));

        Geometry::Line line = {
            Geometry::Vertex{(float)addLineStart.x(), (float)addLineStart.y(), 0., 0., 0.}, 
            Geometry::Vertex{(float)addLineEnd.x(), (float)addLineEnd.y(), 0., 0., 0.}
        };

        _controller->updateLine(line);
    }
}

void AddingLineWithAngleMode::mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx)
{
    _pressed = false;
}

}