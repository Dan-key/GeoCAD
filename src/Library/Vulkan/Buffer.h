#pragma once

#include "Library/Vulkan/VulkanComponent.h"
#include "Library/Vulkan/VulkanManager.h"
#include <cstddef>
#include <memory>
#include <vulkan/vulkan.h>

namespace Vulkan {

class Buffer : protected VulkanComponent {
public:
    Buffer(const std::shared_ptr<VulkanManager>& vkManager, size_t size);

private:
    VkBuffer _vkBuffer;
};

}