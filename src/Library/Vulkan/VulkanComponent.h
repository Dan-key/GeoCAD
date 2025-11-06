#pragma once

#include "Library/Vulkan/VulkanManager.h"

namespace Vulkan {

class VulkanComponent {
protected:
    VulkanComponent(const std::shared_ptr<VulkanManager>& vkManager) : _vkManager(vkManager) {}
    std::shared_ptr<VulkanManager> _vkManager;
};

}