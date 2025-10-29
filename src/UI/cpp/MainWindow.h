#pragma once

#include <iostream>
#include <QObject>

class MainWindow : public QObject
{
    Q_OBJECT
public:
    MainWindow(QObject* parent = nullptr);

public slots:
    void addLine();
signals:
    void lineSignal();

};