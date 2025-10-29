#include "MainWindow.h"
#include <QCursor>
#include <QGuiApplication>

MainWindow::MainWindow(QObject* parent) :
    QObject(parent)
{}

void MainWindow::addLine()
{
    if (!QGuiApplication::overrideCursor() || QGuiApplication::overrideCursor()->shape() != Qt::CrossCursor) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
        emit lineSignal();
    } 
}