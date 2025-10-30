#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QtQml>
#include <vulkan/vulkan.h>
#include "UI/cpp/MainWindow.h"
#include "UI/cpp/VulkanItem.h"
#include <QQuickWindow>
#include "Library/Files/FileStream.h"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("xcb")); 
    qputenv("QT_QPA_PLATFORMTHEME", QByteArray("gnome"));
    QGuiApplication app(argc, argv);
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layer : availableLayers) {
        std::cout << layer.layerName << std::endl;
    }

    QVulkanInstance inst;

    inst.setApiVersion(QVersionNumber(1, 3));

#ifdef QT_DEBUG
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
#endif

    if (!inst.create()) {
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());
        return -1;
    }

    QQuickWindow::setGraphicsApi(QSGRendererInterface::VulkanRhi);
    qmlRegisterType<VulkanItem>("VulkanApp", 1, 0, "VulkanItem");

    MainWindow* mainWindow = new MainWindow;


    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/src/UI/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;
    engine.rootContext()->setContextProperty(QStringLiteral("mainWindow"), mainWindow);
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (window) {
        window->setVulkanInstance(&inst);
    }

    return app.exec();
}