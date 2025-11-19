#pragma once

#include <QPointF>
#include <QSizeF>

struct ViewportContext
{
    float zoomLevel;
    QPointF offset;
    QSizeF viewportSize;
};