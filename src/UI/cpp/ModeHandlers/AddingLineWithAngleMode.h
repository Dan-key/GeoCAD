#pragma once

#include "UI/cpp/ModeHandlers/IModeHandler.h"

namespace ModeHandlers {

class AddingLineWithAngleMode : public IModeHandler
{
public:
    AddingLineWithAngleMode(MainWindow* controller);
    void mousePressEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseMoveEvent(QMouseEvent *event, ViewportContext cntx) override;
    void mouseReleaseEvent(QMouseEvent *event, ViewportContext cntx) override;

private:
    bool _pressed = false;
};

}