#include "Buffer.h"
#include "Library/Vulkan/VulkanManager.h"
#include <cstddef>
#include <memory>

namespace Vulkan {

Buffer::Buffer(const std::shared_ptr<VulkanManager>& vkManager, size_t size) :
    VulkanComponent(vkManager)
{
    _vkBuffer = 
}

}