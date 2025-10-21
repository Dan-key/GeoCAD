#include "VulkanItem.h"
#include <QQuickWindow>
#include <QSGRendererInterface>

// VulkanRenderNode Implementation
VulkanRenderNode::VulkanRenderNode(QQuickItem *item)
    : m_item(item)
{
}

VulkanRenderNode::~VulkanRenderNode()
{
    releaseResources();
}

void VulkanRenderNode::initVulkan()
{
    if (m_initialized)
        return;

    QQuickWindow *window = m_item->window();
    if (!window)
        return;

    QSGRendererInterface *rif = window->rendererInterface();
    if (rif->graphicsApi() != QSGRendererInterface::VulkanRhi)
        return;

    // Get Vulkan device from Qt Quick's scene graph
    m_device = *static_cast<VkDevice *>(
        rif->getResource(window, QSGRendererInterface::DeviceResource));

    if (m_device == VK_NULL_HANDLE)
        return;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    auto vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(
        window->vulkanInstance()->getInstanceProcAddr("vkCreateCommandPool"));
    
    if (vkCreateCommandPool)
        vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

    createCommandBuffer();
    m_initialized = true;
}

void VulkanRenderNode::createCommandBuffer()
{
    if (m_commandPool == VK_NULL_HANDLE)
        return;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    QQuickWindow *window = m_item->window();
    auto vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(
        window->vulkanInstance()->getInstanceProcAddr("vkAllocateCommandBuffers"));
    
    if (vkAllocateCommandBuffers)
        vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
}

void VulkanRenderNode::render(const RenderState *state)
{
    if (!m_initialized)
        initVulkan();

    if (m_commandBuffer == VK_NULL_HANDLE)
        return;

    // Record your Vulkan rendering commands here
    recordCommandBuffer();

    // The command buffer will be submitted by Qt Quick's scene graph
}

void VulkanRenderNode::recordCommandBuffer()
{
    // Example: Begin command buffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    // Add your Vulkan rendering code here
    // For example: vkCmdDraw, vkCmdDrawIndexed, etc.
}

void VulkanRenderNode::releaseResources()
{
    if (m_commandBuffer != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        QQuickWindow *window = m_item->window();
        if (window) {
            auto vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(
                window->vulkanInstance()->getInstanceProcAddr("vkFreeCommandBuffers"));
            if (vkFreeCommandBuffers)
                vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
        }
        m_commandBuffer = VK_NULL_HANDLE;
    }

    if (m_commandPool != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        QQuickWindow *window = m_item->window();
        if (window) {
            auto vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(
                window->vulkanInstance()->getInstanceProcAddr("vkDestroyCommandPool"));
            if (vkDestroyCommandPool)
                vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        }
        m_commandPool = VK_NULL_HANDLE;
    }

    m_initialized = false;
}

QSGRenderNode::StateFlags VulkanRenderNode::changedStates() const
{
    return BlendState | StencilState | DepthState | ScissorState | ColorState;
}

QSGRenderNode::RenderingFlags VulkanRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

// VulkanItem Implementation
VulkanItem::VulkanItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

VulkanItem::~VulkanItem()
{
}

QSGNode *VulkanItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    VulkanRenderNode *node = static_cast<VulkanRenderNode *>(oldNode);
    
    if (!node) {
        node = new VulkanRenderNode(this);
    }

    return node;
}

void VulkanItem::ensureVulkanInstance()
{
    if (window()) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (!inst) {
            qWarning("VulkanItem: No Vulkan instance found on window!");
        }
    }
}