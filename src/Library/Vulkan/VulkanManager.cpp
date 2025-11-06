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
    _device = static_cast<VkDevice>(_rif->getResource(_itemWindow, QSGRendererInterface::Resource::DeviceResource));
    _physicalDevice = static_cast<VkPhysicalDevice>(_rif->getResource(_itemWindow, QSGRendererInterface::Resource::PhysicalDeviceResource));

}

VkShaderModule VulkanManager::createShaderModule(const SpirvByteCode& spirv) const
{
    VkShaderModule shader;

    VkShaderModuleCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size();
    createInfo.pCode = spirv.data();

    VkResult result = _devFuncs->vkCreateShaderModule(_device, &createInfo, nullptr, &shader);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(QString("can't create shader module, result: %1").arg(result).toStdString());
    }
    
    return shader;
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

VkBuffer VulkanManager::createBuffer(size_t size) const
{
    VkBuffer buffer;
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = _devFuncs->vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(QString("can't create buffer result: %1").arg(result).toStdString());
    }

    return buffer;
}


} // namespace Vulkan