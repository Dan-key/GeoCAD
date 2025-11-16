#pragma once

#include <future>
#include <memory>
#include <vulkan/vulkan.h>
#include <string>
#include "Library/Vulkan/SpirvByteCode.h"
#include "Library/Vulkan/VulkanComponent.h"
#include "Library/Vulkan/VulkanManager.h"

namespace Vulkan {

class ShaderModule : protected VulkanComponent {
public:
    ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const std::string& fileName);
    ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const SpirvByteCode&);

    ~ShaderModule();

    void setShader(const SpirvByteCode& spirv);

    ShaderModule(std::shared_ptr<VulkanManager>& vkManager);

    operator VkShaderModule() { return _module; };
    VkShaderModule get();

protected:
    VkShaderModule _module = VK_NULL_HANDLE;
};

}