#pragma once

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>
#include <qpoint.h>

#include "VulkanRenderNode.h"


class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    // Add properties for mouse interaction
    // Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    // Q_PROPERTY(QColor triangleColor READ triangleColor WRITE setTriangleColor NOTIFY triangleColorChanged)

public:
    VulkanItem(QQuickItem *parent = nullptr);

    // bool interactive() const { return m_interactive; }
    // void setInteractive(bool interactive);

    // QColor triangleColor() const { return m_triangleColor; }
    // void setTriangleColor(const QColor& color);
    static float z;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    
    // Mouse event handlers
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void interactiveChanged();
    void triangleColorChanged();
    void mousePressed(qreal x, qreal y);
    void mouseMoved(qreal x, qreal y);
    void mouseReleased(qreal x, qreal y);
    void hoverPositionChanged(qreal x, qreal y);

private:
    VulkanRenderNode *m_renderNode = nullptr;
    // bool m_interactive = true;
    // QColor m_triangleColor = Qt::red;
    bool m_mousePressed = false;
    QPointF deltaPos;
    QPointF beginPos;
};
