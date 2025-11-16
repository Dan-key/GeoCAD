#include <cstdint>
#include <vulkan/vulkan.h>
#include <stdexcept>
#include "Buffer.h"
#include "Library/Vulkan/VulkanManager.h"
#include <cstddef>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Vulkan {

Buffer::Buffer(const std::shared_ptr<VulkanManager>& vkManager) :
    VulkanComponent(vkManager)
{
}

void Buffer::allocateMemory(size_t size)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.pNext = nullptr;

    _vkBuffer = _vkManager->createBuffer(&bufferInfo);

    vkGetBufferMemoryRequirements(_vkManager->device(), _vkBuffer,  &_memReq);
    _memoryIndex = findMemoryType(_vkManager->physicalDevice(), _memReq.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.memoryTypeIndex = _memoryIndex;
    allocInfo.allocationSize = _memReq.size;
    allocInfo.pNext = nullptr;

    VkDeviceMemory devMemory;

    VkResult res = vkAllocateMemory(_vkManager->device(), &allocInfo, nullptr, &devMemory);
    if (res != VK_SUCCESS) {
        throw std::runtime_error(QString("can't allocate memory with size %1, return: %2").arg(size).arg(res).toStdString());
    }
    _memory = devMemory;
    vkBindBufferMemory(_vkManager->device(), _vkBuffer, devMemory, 0);
}

void Buffer::updateMemory(uint32_t offsetInBytes, void* data, uint32_t size)
{
    void* d;
    VkResult res = vkMapMemory(_vkManager->device(), _memory, offsetInBytes, size, 0, &d);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("can't map error");
    }
    memcpy(d, data, size);
    vkUnmapMemory(_vkManager->device(), _memory);
}

uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t memoryBits, int flagBits) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, & memProperties);

    for (uint32_t i  = 0; i < memProperties.memoryTypeCount; ++i) {
        if (((1 << i) & memoryBits) && ((memProperties.memoryTypes[i].propertyFlags & flagBits)) == flagBits) {
            return i;
        }
    }
    return 0;
}

Buffer::~Buffer()
{
    vkFreeMemory(_vkManager->device(), _memory, nullptr);
    vkDestroyBuffer(_vkManager->device(), _vkBuffer, nullptr);
}

}