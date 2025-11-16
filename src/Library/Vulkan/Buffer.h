#pragma once

#include "Library/Vulkan/VulkanComponent.h"
#include "Library/Vulkan/VulkanManager.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkan {

class Buffer : protected VulkanComponent {
public:
    Buffer(const std::shared_ptr<VulkanManager>& vkManager);
    ~Buffer();
    // returns index of memory (needed for update)
    void allocateMemory(size_t size);
    void updateMemory(uint32_t offsetInBytes, void* data, uint32_t size);

    operator VkBuffer() { return _vkBuffer; }

private:
    VkBuffer _vkBuffer;
    VkMemoryRequirements _memReq;
    uint32_t _memoryIndex;
    VkDeviceMemory _memory;

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryBits, int flags) const;

};

}