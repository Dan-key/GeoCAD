#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QtQml>
#include <vulkan/vulkan.h>
#include "UI/cpp/VulkanItem.h"
#include <QQuickWindow>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layer : availableLayers) {
        std::cout << layer.layerName << std::endl;
    }
    QQuickWindow::setGraphicsApi(QSGRendererInterface::VulkanRhi);
    qmlRegisterType<VulkanItem>("VulkanApp", 1, 0, "VulkanItem");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/src/UI/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}