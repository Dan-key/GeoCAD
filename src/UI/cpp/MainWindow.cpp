#include "MainWindow.h"
#include <QCursor>
#include <QGuiApplication>
#include <memory>
#include "UI/cpp/Geometry/Vertex.h"
#include "UI/cpp/ModeHandlers/ModeHandlers.h"
#include "UI/cpp/VulkanRenderNode.h"


MainWindow::MainWindow(QObject* parent) :
    QObject(parent),
    lines(Flux::MutableList<Geometry::Line>()),
    _modeController(std::make_shared<ModeHandlers::IModeHandler>(this))
{}

void MainWindow::addLine(const Geometry::Line& line)
{
    lines.add(line);
}

void MainWindow::updateLine(const Geometry::Line& line)
{
    lines.update(lines.get().size() -1, line);
}

void MainWindow::updatePosition(const QPointF& position)
{
    VulkanRenderNode::pos = position;
}

void MainWindow::mouseMove(QMouseEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->mouseMoveEvent(event, cntx);
    }
}

void MainWindow::mousePress(QMouseEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->mousePressEvent(event, cntx);
    }
}

void MainWindow::mouseRelease(QMouseEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->mouseReleaseEvent(event, cntx);
    }
}

void MainWindow::hoverEnter(QHoverEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->hoverEnterEvent(event, cntx);
    }
}

void MainWindow::hoverMove(QHoverEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->hoverMoveEvent(event, cntx);
    }
}

void MainWindow::hoverLeave(QHoverEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->hoverLeaveEvent(event, cntx);
    }
}

void MainWindow::wheel(QWheelEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->wheelEvent(event, cntx);
    }
}

void MainWindow::keyPress(QKeyEvent* event, ViewportContext cntx)
{
    if (_modeController) {
        _modeController->keyPressEvent(event, cntx);
    }
}

void MainWindow::changeMode(Mode newMode)
{
    currentMode = newMode;
    qDebug() << "Changing mode to" << static_cast<int>(currentMode);
    switch (currentMode) {
        case Mode::None:
            QGuiApplication::restoreOverrideCursor();
            _modeController = std::make_shared<ModeHandlers::IModeHandler>(this);
            break;
        case Mode::AddLine:
            if (!QGuiApplication::overrideCursor() || QGuiApplication::overrideCursor()->shape() != Qt::CrossCursor) {
                QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
            }
            _modeController = std::make_shared<ModeHandlers::AddingLineMode>(this);
    }
}

void MainWindow::addingLineWithCoordinates(float x1, float y1, float x2, float y2)
{
    addLine(Geometry::Line{Geometry::Vertex{x1, y1, 0, 0, 0}, Geometry::Vertex{x2, y2, 0, 0, 0}});
}


void MainWindow::addLineMode()
{
    changeMode(Mode::AddLine);
}
