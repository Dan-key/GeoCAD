#include "ShaderModule.h"
#include "Library/Files/FileStream.h"
#include "Library/Vulkan/SpirvByteCode.h"
#include "Library/Vulkan/VulkanManager.h"

namespace Vulkan {

ShaderModule::ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const SpirvByteCode& spirv) :
    _vkManager(vkManager)
{
    _module = _vkManager->createShaderModule(spirv);
}

ShaderModule::ShaderModule(std::shared_ptr<VulkanManager>& vkManager, const std::string& fileName)
{
    Files::FileStream fs(fileName);
    SpirvByteCode spirv = fs.getSpirvByteCode();

    _module = vkManager->createShaderModule(spirv);
}

}