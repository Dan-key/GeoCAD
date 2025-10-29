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
#include "VulkanRenderNode.h"

void VulkanItem::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;
    QPointF localPos = event->position();

    qDebug() << "Mouse pressed at:" << localPos << "buttons" << event->buttons();
    emit mousePressed(localPos.x(), localPos.y());
    if (isLineAdding) {
        if (event->buttons() & Qt::RightButton) {
            isLineAdding = false;
            isSecondPoint = false;
            QGuiApplication::restoreOverrideCursor();
            event->accept();
            return;
        }
        if (!isSecondPoint) {
            auto itemSize = size();

            addLineStart = QPointF(
                (((event->position().x() - VulkanRenderNode::pos.x() / 2) * 2 / (itemSize.width()) - 1) / VulkanRenderNode::z),
                (((event->position().y() - VulkanRenderNode::pos.y() / 2) * 2 / (itemSize.height()) - 1) / VulkanRenderNode::z)
            );
            qDebug() << "addLineStart" << addLineStart << "eventposition" << event->position();
            // isSecondPoint = true;
            return;
        }
    }
    if (m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    } else if (m_renderNode && (event->buttons() & Qt::MiddleButton || event->buttons() & Qt::RightButton)) {
        beginPos = event->position();
        deltaPos = VulkanRenderNode::pos;
    }

    event->accept();
}

void VulkanItem::mouseMoveEvent(QMouseEvent *event)
{
    QPointF localPos = event->position();

    if (isLineAdding) {
        auto itemSize = size();

        float endXpos = ((localPos.x() - VulkanRenderNode::pos.x() / 2) * 2 / itemSize.width() - 1) / VulkanRenderNode::z;
        float endYpos = ((localPos.y() - VulkanRenderNode::pos.y() / 2) * 2 / itemSize.height() - 1) / VulkanRenderNode::z;
        Geometry::Line line = {
            Geometry::Vertex{(float)addLineStart.x(), (float)addLineStart.y(), 0., 0., 0.}, 
            Geometry::Vertex{endXpos, endYpos, 0., 0., 0.}
        };
        if (isSecondPoint) {
            m_renderNode->updateLine(line);
        } else {
            isSecondPoint = true;
            m_renderNode->addLine(line);
        }
    }
    if (m_mousePressed && m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    } else if (m_mousePressed && m_renderNode && (event->buttons() & Qt::MiddleButton || event->buttons() & Qt::RightButton)) {
        auto delta = event->position() - beginPos;
        VulkanRenderNode::pos = deltaPos + delta;
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
    beginPos = {0.0, 0.0};
    deltaPos = {0.0, 0.0};
    
    if (isLineAdding) {
        if (isSecondPoint) {
        auto itemSize = size();

        float endXpos = ((event->position().x() - VulkanRenderNode::pos.x() / 2) * 2 / itemSize.width() - 1) / VulkanRenderNode::z;
        float endYpos = ((event->position().y() - VulkanRenderNode::pos.y() / 2) * 2 / itemSize.height() - 1) / VulkanRenderNode::z;

        qDebug() << "endLineStart" << QPointF{endXpos, endXpos} << "eventposition" << event->position();

        Geometry::Line line = {
            Geometry::Vertex{(float)addLineStart.x(), (float)(addLineStart.y()),0.0, 0.0, 0.0},
            Geometry::Vertex{endXpos, endYpos, 0.0, 0.0, 0.0}};
        m_renderNode->addLine(line);
        isSecondPoint = false;
        isLineAdding = false;
        QGuiApplication::restoreOverrideCursor();
        event->accept();
        return;
        } else {
            isSecondPoint = true;
        }
    }

    event->accept();
}

void VulkanItem::hoverEnterEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    emit hoverPositionChanged(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::keyPressEvent(QKeyEvent* event)
{
    qDebug() << event->key();
    if (event->key() & Qt::Key_Escape) {
        isLineAdding = false;
        QGuiApplication::restoreOverrideCursor();
    }
    event->accept();
}

void VulkanItem::hoverMoveEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    // qDebug() << "hover" << localPos;
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
    if (delta > 0) {
        VulkanRenderNode::z /= 0.9;
    } else if (delta < 0 && VulkanRenderNode::z > 0.04) {
        VulkanRenderNode::z *= 0.9;
    }
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

QSGNode* VulkanItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    VulkanRenderNode *node = static_cast<VulkanRenderNode *>(oldNode);
    if (!node)
        node = new VulkanRenderNode(this);
    m_renderNode = node;
    node->markDirty(QSGNode::DirtyMaterial);

    return node;
}

void VulkanItem::addingLine()
{
    isLineAdding = true;
    qDebug() << "get line signal";
}