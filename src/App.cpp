#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include "UI/cpp/VulkanItem.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<VulkanItem>("VulkanApp", 1, 0, "VulkanItem");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/src/UI/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}