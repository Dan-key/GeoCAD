#pragma once

#include "IModeHandler.h"

namespace ModeHandlers {

class AddingLineMode : public IModeHandler
{
public:
    AddingLineMode(MainWindow* controller);
    void mousePressEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseMoveEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx) override;
    // void hoverEnterEvent(QHoverEvent *event, ViewportContext cntx) override;
    // void hoverMoveEvent(QHoverEvent *event, ViewportContext cntx) override;
    // void hoverLeaveEvent(QHoverEvent *event, ViewportContext cntx) override;
    // void wheelEvent(QWheelEvent *event, ViewportContext cntx) override;
    // void keyPressEvent(QKeyEvent* event, ViewportContext cntx) override;
private:
    bool m_mouseLinePressed = false;
    QPointF addLineStart;
    bool isSecondPoint = false;

};

} // namespace ModeHandlers