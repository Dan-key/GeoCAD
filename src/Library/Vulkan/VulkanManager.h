#pragma once

#include <QQuickItem>
#include <QVulkanInstance>
#include <QSGRendererInterface>
#include "SpirvByteCode.h"

namespace Vulkan {

class VulkanManager {
public:
    VulkanManager(QQuickItem* item);

    VkShaderModule createShaderModule(const SpirvByteCode&) const;
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