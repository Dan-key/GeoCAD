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
#include "VulkanRenderNode.h"

void VulkanItem::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;
    QPointF localPos = event->position();

    qDebug() << "Mouse pressed at:" << localPos;
    emit mousePressed(localPos.x(), localPos.y());

    if (m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    }

    event->accept();
}

void VulkanItem::mouseMoveEvent(QMouseEvent *event)
{
    QPointF localPos = event->position();

    if (m_mousePressed && m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    }

    emit mouseMoved(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePressed = false;
    QPointF localPos = event->position();

    qDebug() << "Mouse released at:" << localPos;
    emit mouseReleased(localPos.x(), localPos.y());

    event->accept();
}

void VulkanItem::hoverEnterEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    emit hoverPositionChanged(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::hoverMoveEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    emit hoverPositionChanged(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::hoverLeaveEvent(QHoverEvent *event)
{
    emit hoverPositionChanged(-1, -1); // Signal that hover left
    event->accept();
}

void VulkanItem::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().ry();
    VulkanRenderNode::z = delta > 0 ? VulkanRenderNode::z / 0.9 : VulkanRenderNode::z * 0.9;
    qDebug() << "VulkanItem::z" << VulkanRenderNode::z;
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

QSGNode *VulkanItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    VulkanRenderNode *node = static_cast<VulkanRenderNode *>(oldNode);
    if (!node)
        node = new VulkanRenderNode(this);
    m_renderNode = node;
    node->markDirty(QSGNode::DirtyMaterial);

    return node;
}