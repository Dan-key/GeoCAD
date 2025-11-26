#pragma once

#include "UI/cpp/ModeHandlers/IModeHandler.h"

namespace ModeHandlers {

class MoveHandler : public IModeHandler
{
public:
    MoveHandler(MainWindow* controller);
    void mousePressEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseMoveEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx) override;
    void wheelEvent(QWheelEvent* event, ViewportContext cntx) override;

private:
    bool m_mousePressed = false;
    QPointF deltaPos;
    QPointF beginPos;
};

}