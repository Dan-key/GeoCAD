#include <qcontainerfwd.h>
#include <vulkan/vulkan.h>
#include "VulkanManager.h"
#include <QQuickWindow>
#include <QVulkanDeviceFunctions>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Vulkan {

VulkanManager::VulkanManager(QQuickItem* item) :
    _item(item)
{
    if (!_item) {
        throw std::runtime_error("item is nullptr");
    }

    _itemWindow = _item->window();
    if (!_itemWindow) {
        throw std::runtime_error("item window is nullptr");
    }

    _rif = _itemWindow->rendererInterface();
    if (!_rif) {
        throw std::runtime_error("render interface is nullptr");
    }

    if (_rif->graphicsApi() != QSGRendererInterface::VulkanRhi) {
        throw std::runtime_error("render interface isn't vulkan");
    }

    _vulkanInstance = _itemWindow->vulkanInstance();
    _devFuncs = static_cast<QVulkanDeviceFunctions *>(_rif->getResource(
        _itemWindow, QSGRendererInterface::Resource::VulkanInstanceResource));
    _device = *static_cast<VkDevice*>(_rif->getResource(_itemWindow, QSGRendererInterface::Resource::DeviceResource));
    _physicalDevice = *static_cast<VkPhysicalDevice*>(_rif->getResource(_itemWindow, QSGRendererInterface::Resource::PhysicalDeviceResource));

}

VkShaderModule VulkanManager::createShaderModule(const SpirvByteCode& spirv) const
{
    VkShaderModule shader;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size();
    createInfo.pCode = spirv.data();

    VkResult result = vkCreateShaderModule(_device, &createInfo, nullptr, &shader);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(QString("can't create shader module, result: %1").arg(result).toStdString());
    }
    
    return shader;
}

void VulkanManager::vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const
{
    ::vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

void VulkanManager::vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const
{
    ::vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

void VulkanManager::vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const
{
    ::vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

void VulkanManager::vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const
{
    ::vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void VulkanManager::vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) const
{
    ::vkCmdSetLineWidth(commandBuffer, lineWidth);
}

void VulkanManager::vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    ::vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanManager::vkDestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) const
{
    ::vkDestroyPipeline(_device, pipeline, pAllocator);
}

void VulkanManager::vkDestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) const
{
    ::vkDestroyPipelineLayout(_device, pipelineLayout, pAllocator);
}

VkResult VulkanManager::vkCreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const
{
    return ::vkCreateGraphicsPipelines(_device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

void VulkanManager::printDebug() const
{
        // Test that device functions work with a simple call
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(_physicalDevice, &props);
    qDebug("GPU: %s", props.deviceName);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(_physicalDevice, &features);
    qDebug("wideline supported: %d", features.wideLines);
}

VkResult VulkanManager::vkCreateCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) const
{
    return ::vkCreateCommandPool(_device, pCreateInfo, pAllocator, pCommandPool);
}

VkResult VulkanManager::vkCreatePipelineLayout(VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) const
{
    return ::vkCreatePipelineLayout(_device, pCreateInfo, pAllocator, pPipelineLayout);
}

VkResult VulkanManager::vkAllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) const
{
    return ::vkAllocateCommandBuffers(_device, pAllocateInfo, pCommandBuffers);
}

void VulkanManager::vkFreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const
{
    ::vkFreeCommandBuffers(_device, commandPool, commandBufferCount, pCommandBuffers);
}

void VulkanManager::vkDestroyCommandPool(VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) const
{
    ::vkDestroyCommandPool(_device, commandPool, pAllocator);
}

void VulkanManager::vkDestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) const
{
    ::vkDestroyShaderModule(_device, shaderModule, pAllocator);
}

VkBuffer VulkanManager::createBuffer(VkBufferCreateInfo* bufferInfo) const
{
    VkBuffer buffer;

    VkResult result = vkCreateBuffer(_device, bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(QString("can't create buffer result: %1").arg(result).toStdString());
    }

    return buffer;
}


} // namespace Vulkan