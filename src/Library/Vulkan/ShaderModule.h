#pragma once

#include <future>
#include <memory>
#include <vulkan/vulkan.h>
#include <string>
#include "Library/Vulkan/SpirvByteCode.h"
#include "Library/Vulkan/VulkanManager.h"

namespace Vulkan {

class ShaderModule {
public:
    ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const std::string& fileName);
    ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const SpirvByteCode&);

    operator VkShaderModule();
    VkShaderModule get();

protected:
    VkShaderModule _module;
    std::shared_ptr<VulkanManager> _vkManager;
};

}