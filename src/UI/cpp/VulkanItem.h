#pragma once

#include <QQuickItem>
#include <QVulkanInstance>
#include <QVulkanWindowRenderer>
#include <QVulkanDeviceFunctions>
#include <QVulkanWindow>
#include <QSGTexture>
#include <QTimer>

class VulkanRenderer;

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    VulkanWindow();
    QVulkanWindowRenderer *createRenderer() override;
    QImage getRenderedImage();
    
    void renderFrame();

private:
    friend class VulkanRenderer;
    VulkanRenderer* m_renderer = nullptr;
    QImage m_currentFrame;
};

class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    VulkanItem(QQuickItem *parent = nullptr);
    ~VulkanItem();

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;

private slots:
    void handleWindowChanged(QQuickWindow *win);
    void updateTexture();

private:
    void setupVulkanWindow();
    
    QVulkanInstance m_instance;
    VulkanWindow *m_vulkanWindow = nullptr;
    QTimer *m_updateTimer = nullptr;
    bool m_vulkanInitialized = false;
};