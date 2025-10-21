#ifndef VULKANITEM_H
#define VULKANITEM_H

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>

class VulkanRenderNode : public QSGRenderNode
{
public:
    VulkanRenderNode(QQuickItem *item);
    ~VulkanRenderNode();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;

private:
    void initVulkan();
    void createCommandBuffer();
    void recordCommandBuffer();

    QQuickItem *m_item;
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    bool m_initialized = false;
};

class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    VulkanItem(QQuickItem *parent = nullptr);
    ~VulkanItem();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    void ensureVulkanInstance();
};

#endif // VULKANITEM_H