#pragma once

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>
#include <qevent.h>
#include <qpoint.h>

#include "UI/cpp/MainWindow.h"
#include "UI/cpp/ModeHandlers/ViewportContext.h"
#include "VulkanRenderNode.h"


class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    // Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    // Q_PROPERTY(QColor triangleColor READ triangleColor WRITE setTriangleColor NOTIFY triangleColorChanged)
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)

public:
    VulkanItem(QQuickItem *parent = nullptr);

    // bool interactive() const { return m_interactive; }
    // void setInteractive(bool interactive);

    // QColor triangleColor() const { return m_triangleColor; }
    // void setTriangleColor(const QColor& color);
    static float z;

    QObject* controller() const { return _controller; }
    void setController(QObject* controller);

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
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void interactiveChanged();
    void mousePress(QMouseEvent* event, ViewportContext cntx);
    void mouseMove(QMouseEvent* event, ViewportContext cntx);
    void mouseRelease(QMouseEvent* event, ViewportContext cntx);
    void hoverEnter(QHoverEvent* event, ViewportContext cntx);
    void hoverMove(QHoverEvent* event, ViewportContext cntx);
    void hoverLeave(QHoverEvent* event, ViewportContext cntx);
    void wheel(QWheelEvent* event, ViewportContext cntx);
    void keyPress(QKeyEvent* event, ViewportContext cntx);
    void controllerChanged();

private:
    VulkanRenderNode *m_renderNode = nullptr;

    // bool m_interactive = true;
    // QColor m_triangleColor = Qt::red;
    bool m_mousePressed = false;
    MainWindow *_controller = nullptr;
    QPointF deltaPos;
    QPointF beginPos;
    bool isLineAdding = false;

    QPointF addLineStart;
    bool isSecondPoint = false;
public slots:
    // void addLine(const Geometry::Line& line);
    // void updateLine(const Geometry::Line& line);
    // void addingLineWithCoordinates(float x1, float y1, float x2, float y2);
private:
    void connectController(MainWindow* controller);
};
