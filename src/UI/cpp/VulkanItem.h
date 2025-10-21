#ifndef VULKANITEM_H
#define VULKANITEM_H

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>

struct Vertex {
    float pos[2];
    float color[3];
};

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
    void createCommandPool();
    void createVertexBuffer();
    void createShaderModules();
    void createPipeline(VkRenderPass renderPass);
    void recordCommandBuffer(const RenderState *state);

    QQuickItem *m_item;
    QVulkanInstance *m_vulkanInstance = nullptr;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    
    VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;
    
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    
    bool m_initialized = false;
    bool m_pipelineCreated = false;
    uint32_t m_queueFamilyIndex = 0;
};

class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    VulkanItem(QQuickItem *parent = nullptr);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
};

#endif // VULKANITEM_H