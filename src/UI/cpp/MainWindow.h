#pragma once

#include <iostream>
#include <QObject>

#include "Library/Meta/Meta.h"

class MainWindow : public QObject
{
    Q_OBJECT
public:
    MainWindow(QObject* parent = nullptr);

    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;

    EXPOSE(x1);
    EXPOSE(y1);
    EXPOSE(x2);
    EXPOSE(y2);

public slots:
    void addLine();
    void addLineWithCoordinates();
signals:
    void lineSignal();

};