#include "ShaderModule.h"
#include "Library/Files/FileStream.h"
#include "Library/Vulkan/SpirvByteCode.h"
#include "Library/Vulkan/VulkanComponent.h"
#include "Library/Vulkan/VulkanManager.h"

namespace Vulkan {

ShaderModule::ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const SpirvByteCode& spirv) :
    VulkanComponent(vkManager)
{
    _module = _vkManager->createShaderModule(spirv);
}

ShaderModule::ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const std::string& fileName) :
    VulkanComponent(vkManager)
{
    Files::FileStream fs(fileName);
    SpirvByteCode spirv = fs.getSpirvByteCode();

    _module = vkManager->createShaderModule(spirv);
}

ShaderModule::ShaderModule(std::shared_ptr<VulkanManager>& vkManager) :
    VulkanComponent(vkManager)
{

}

void ShaderModule::setShader(const SpirvByteCode& spirv)
{
    _module = _vkManager->createShaderModule(spirv);
}

ShaderModule::~ShaderModule()
{
    if (_module != VK_NULL_HANDLE) {
        _vkManager->vkDestroyShaderModule(_module, nullptr);
        _module = VK_NULL_HANDLE;
    }
}

} //namespace Vulkan