#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vulkan/vulkan.h>
#include "VulkanItem.h"
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <cstring>
#include <vector>
#include <iostream>
#include <QTimer>

#include "Geometry/Vertex.h"
#include "UI/cpp/MainWindow.h"
#include "VulkanRenderNode.h"

void VulkanItem::mousePressEvent(QMouseEvent *event)
{
    emit mousePress(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMove(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseRelease(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::hoverEnterEvent(QHoverEvent *event)
{
    emit hoverEnter(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::setController(QObject* controller)
{
    if (_controller == controller)
        return;

    _controller = static_cast<MainWindow*>(controller);
    emit controllerChanged();

    if (_controller) {
        connectController(_controller);
    }
}

void VulkanItem::keyPressEvent(QKeyEvent* event)
{
    qDebug() << event->key();
    if (event->key() & Qt::Key_Escape) {
        isLineAdding = false;
        QGuiApplication::restoreOverrideCursor();
    }
    emit keyPress(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::hoverMoveEvent(QHoverEvent *event)
{
    emit hoverMove(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::hoverLeaveEvent(QHoverEvent *event)
{
    emit hoverLeave(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

void VulkanItem::wheelEvent(QWheelEvent* event)
{
    qDebug() << "VulkanItem::z" << VulkanRenderNode::z;
    emit wheel(event, ViewportContext{VulkanRenderNode::z, VulkanRenderNode::pos, size()});
    event->accept();
}

// VulkanItem Implementation
VulkanItem::VulkanItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VulkanItem::update);
    timer->start(10); 
}

QSGNode* VulkanItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    VulkanRenderNode *node = static_cast<VulkanRenderNode *>(oldNode);
    if (!node)
        node = new VulkanRenderNode(this, _controller);
    m_renderNode = node;
    node->markDirty(QSGNode::DirtyMaterial);

    return node;
}

// void VulkanItem::addingLine()
// {
//     isLineAdding = true;
//     qDebug() << "get line signal";
// }

// void VulkanItem::addingLineWithCoordinates(float x1, float y1, float x2, float y2)
// {
//     // isLineAdding = false;
//     // qDebug() << "get line with coordinates signal";

//     // Geometry::Line line = {
//     //     Geometry::Vertex{(float)x1, (float)y1, 0., 0., 0.}, 
//     //     Geometry::Vertex{(float)x2, (float)y2, 0., 0., 0.}
//     // };
//     // if (m_renderNode) {
//     //     m_renderNode->addLine(line);
//     // }
// }

void VulkanItem::connectController(MainWindow* controller)
{
    qRegisterMetaType<ViewportContext>("ViewportContext");

    QObject::connect(this, &VulkanItem::mousePress, controller, &MainWindow::mousePress);
    QObject::connect(this, &VulkanItem::mouseMove, controller, &MainWindow::mouseMove);
    QObject::connect(this, &VulkanItem::mouseRelease, controller, &MainWindow::mouseRelease);
    QObject::connect(this, &VulkanItem::hoverEnter, controller, &MainWindow::hoverEnter);
    QObject::connect(this, &VulkanItem::hoverMove, controller, &MainWindow::hoverMove);
    QObject::connect(this, &VulkanItem::hoverLeave, controller, &MainWindow::hoverLeave);
    QObject::connect(this, &VulkanItem::wheel, controller, &MainWindow::wheel);
    QObject::connect(this, &VulkanItem::keyPress, controller, &MainWindow::keyPress);
}