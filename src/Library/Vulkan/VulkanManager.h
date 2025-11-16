#pragma once

#include <QQuickItem>
#include <QVulkanInstance>
#include <QSGRendererInterface>
#include <cstddef>
#include <qquickitem.h>
#include "SpirvByteCode.h"

namespace Vulkan {

class VulkanManager {
public:
    VulkanManager(QQuickItem* item);

    VkShaderModule createShaderModule(const SpirvByteCode&) const;
    VkBuffer createBuffer(VkBufferCreateInfo* bufferInfo) const;

    VkResult vkCreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) const;
    VkResult vkCreatePipelineLayout(VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) const;
    VkResult vkAllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) const;
    VkResult vkCreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const;

    void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const;
    void vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const;
    void vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const;
    void vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) const;
    void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const;
    void vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;

    void vkFreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
    void vkDestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) const;
    void vkDestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) const;
    void vkDestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) const;
    void vkDestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) const;

    inline constexpr VkDevice device()
    {
        return _device;
    }

    inline constexpr VkPhysicalDevice physicalDevice()
    {
        return _physicalDevice;
    }

    inline constexpr QVulkanDeviceFunctions* devFuncs()
    {
        return _devFuncs;
    }

    template<typename T>
    T* getResource(QSGRendererInterface::Resource resource)
    {
        return static_cast<T*>(_rif->getResource(_itemWindow, resource));
    }

    inline constexpr QQuickItem* item() 
    {
        return _item;
    }

    inline constexpr QQuickWindow* itemWindow() 
    {
        return _itemWindow;
    }

    void printDebug() const;

protected:
    QQuickItem* _item;
    QQuickWindow* _itemWindow;
    QVulkanInstance* _vulkanInstance = nullptr;
    QVulkanDeviceFunctions* _devFuncs = nullptr;
    QSGRendererInterface* _rif;
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
};

}