#include "MainWindow.h"
#include <QCursor>
#include <QGuiApplication>

MainWindow::MainWindow(QObject* parent) :
    QObject(parent)
{}

void MainWindow::addLine()
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
    emit lineSignal();
}