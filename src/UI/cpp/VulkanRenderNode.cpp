#include <cstddef>
#include <exception>
#include <memory>
#include <qlogging.h>
#include <qquickitem.h>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include <QSGRendererInterface>
#include <iostream>
#include <QQuickWindow>

#include "VulkanRenderNode.h"
#include "Library/Files/FileStream.h"
#include "Library/Vulkan/VulkanManager.h"
#include "UI/cpp/Geometry/Line.h"
#include "UI/cpp/MainWindow.h"

namespace {

std::string queueFamiliesFlagsToString(VkQueueFlags flags)
{
    std::string res = "";
    if (flags & VK_QUEUE_GRAPHICS_BIT) {
        res += "GRAPHICS ";
    }
    if (flags & VK_QUEUE_COMPUTE_BIT) {
        res += "COMPUTE ";
    }
    if (flags & VK_QUEUE_TRANSFER_BIT) {
        res += "TRANSFER ";
    }
    if (flags & VK_QUEUE_SPARSE_BINDING_BIT) {
        res += "SPARSE_BINDING ";
    }
    return res;
}

}

float VulkanRenderNode::z = 1.0f;
QPointF VulkanRenderNode::pos = {0, 0};

VulkanRenderNode::VulkanRenderNode(QQuickItem *item, MainWindow* controller) :
    _vkManager(std::make_shared<Vulkan::VulkanManager>(item)),
    bufferTriangle(_vkManager),
    bufferLine(_vkManager),
    bufferNet(_vkManager),
    bufferAddedLines(_vkManager),
    m_vertShaderModule(_vkManager),
    m_fragShaderModule(_vkManager),
    m_fragDashShaderModule(_vkManager),
    m_vertCircleModule(_vkManager),
    m_fragCircleModule(_vkManager),
    m_verticesAddedLines()
{
    Files::FileStream vertFS("shaders/vertex.spv");
    Files::FileStream fragFS("shaders/frag.spv");
    Files::FileStream fragDashFS("shaders/frag_dash_line.spv");
    Files::FileStream vertCircleFS("shaders/vertex_circle.spv");
    Files::FileStream fragCircleFS("shaders/frag_circle.spv");

    vertShaderCode = vertFS.getSpirvByteCode();
    fragShaderCode = fragFS.getSpirvByteCode();
    fragDashShaderCode = fragDashFS.getSpirvByteCode();
    vertCircleShaderCode = vertCircleFS.getSpirvByteCode();
    fragCircleShaderCode = fragCircleFS.getSpirvByteCode();

    initVulkan(item);
    connectController(controller);
}

VulkanRenderNode::~VulkanRenderNode()
{
    releaseResources();
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

uint32_t findGraphicsQueueFamily(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    uint32_t res = 0;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        qDebug() << "family i " << i << " count " <<queueFamilies[i].queueCount << " flags " << queueFamiliesFlagsToString(queueFamilies[i].queueFlags); 
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            res = i;
        }
    }
    return res;
}

void VulkanRenderNode::initVulkan(QQuickItem* item)
{
    if (m_initialized)
        return;

    _vkManager->printDebug();

    createCommandPool();
    if (m_commandPool == VK_NULL_HANDLE || m_commandBuffer == VK_NULL_HANDLE) {
        qWarning("Failed to create command pool or buffer!");
        return;
    }

    createBuffer();

    // createTriangleVertexBuffer();
    // if (m_vertexTriangleBuffer == VK_NULL_HANDLE) {
    //     qWarning("Failed to create triangle vertex buffer!");
    //     return;
    // }

    // createLineVertexBuffer();
    // if (m_vertexLineBuffer == VK_NULL_HANDLE) {
    //     qWarning("Failed to create line vertex buffer!");
    //     return;
    // }

    // createNetVertexBuffer();
    // if (m_vertexNetBuffer == VK_NULL_HANDLE) {
    //     qWarning("Failed to create net vertex buffer!");
    //     return;
    // }

    // createAddedLinesVertexBuffer();
    // if (m_vertexAddedLinesBuffer == VK_NULL_HANDLE) {
    //     qWarning("Failed to create net vertex buffer!");
    //     return;
    // }

    createShaderModules();
    if (m_vertShaderModule == VK_NULL_HANDLE ||
        m_fragShaderModule == VK_NULL_HANDLE ||
        m_fragCircleModule == VK_NULL_HANDLE ||
        m_fragCircleModule == VK_NULL_HANDLE) {
        qWarning("Failed to create shader modules!");
        return;
    }

    qDebug("Vulkan initialization successful!");
    m_initialized = true;
}

void VulkanRenderNode::createCommandPool()
{
    // Find graphics queue family
    uint32_t queueFamilyIndex = findGraphicsQueueFamily(_vkManager->physicalDevice());

    // Get the graphics queue
    // _vkManager->devFuncs()->vkGetDeviceQueue(_vkManager->device(), queueFamilyIndex, 0, &m_graphicsQueue);

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = _vkManager->vkCreateCommandPool(&poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create command pool: %d", result);
        return;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = _vkManager->vkAllocateCommandBuffers(&allocInfo, &m_commandBuffer);
    if (result != VK_SUCCESS) {
        qWarning("Failed to allocate command buffer: %d", result);
    }
}

void VulkanRenderNode::createBuffer()
{
    m_verticesTriangle = {
        {{0.0f, -0.05f}, {1.0f, 0.0f, 0.0f}},  // Top - Red
        {{0.05f, 0.05f}, {0.0f, 1.0f, 0.0f}},   // Right - Green
        {{-0.05f, 0.05f}, {0.0f, 0.0f, 1.0f}}   // Left - Blue
    };

    bufferTriangle.allocateMemory(m_verticesTriangle.size() * sizeof(decltype(m_verticesTriangle)::value_type));
    bufferTriangle.updateMemory(0, m_verticesTriangle.data(),
                        m_verticesTriangle.size() * sizeof(decltype(m_verticesTriangle)::value_type));

    m_verticesLine = {
        {{0.0f, -100.0f}, {0.2f, 0.2f, 0.7f}},
        {{0.0f, 100.0f}, {0.2f, 0.2f, 0.7f}},
        {{-100.0f, 0.0f}, {0.2f, 0.2f, 0.7f}},
        {{100.0f, 0.0f}, {0.2f, 0.2f, 0.7f}}
    };

    bufferLine.allocateMemory(m_verticesLine.size() * sizeof(decltype(m_verticesLine)::value_type));
    bufferLine.updateMemory(0, m_verticesLine.data(),
                        m_verticesLine.size() * sizeof(decltype(m_verticesLine)::value_type));

    for (float i = -100; i <= 100; i+=0.1) {
        m_verticesNet.push_back({{i, 100.0f}, {0.2f, 0.2f, 0.6f}});
        m_verticesNet.push_back({{i, -100.0f}, {0.2f, 0.2f, 0.6f}});

        m_verticesNet.push_back({{100.0f, i}, {0.2f, 0.2f, 0.6f}});
        m_verticesNet.push_back({{-100.0f, i}, {0.2f, 0.2f, 0.6f}});
    }

    bufferNet.allocateMemory(m_verticesNet.size() * sizeof(decltype(m_verticesNet)::value_type));
    bufferNet.updateMemory(0, m_verticesNet.data(),
                        m_verticesNet.size() * sizeof(decltype(m_verticesNet)::value_type));

    bufferAddedLines.allocateMemory(1000 * sizeof(decltype(m_verticesAddedLines)::value_type));
}

void VulkanRenderNode::createShaderModules()
{
    m_vertShaderModule.setShader(vertShaderCode);
    m_fragShaderModule.setShader(fragShaderCode);
    m_fragDashShaderModule.setShader(fragDashShaderCode);
    m_vertCircleModule.setShader(vertCircleShaderCode);
    m_fragCircleModule.setShader(fragCircleShaderCode);
}

void VulkanRenderNode::createTrianglePipeline(VkRenderPass renderPass)
{
    // Verify we have a valid render pass
    if (renderPass == VK_NULL_HANDLE) {
        qWarning("Cannot create pipeline: invalid render pass");
        return;
    }

    // Don't recreate if already created
    if (m_trianglePipelineCreated && m_graphicsTrianglePipeline != VK_NULL_HANDLE) {
        return;
    }

    qDebug("Creating graphics pipeline with render pass %p", renderPass);

    // Verify all prerequisites
    // if (!m_devFuncs) {
    //     qWarning("No device functions!");
    //     return;
    // }

    if (_vkManager->device() == VK_NULL_HANDLE) {
        qWarning("No device!");
        return;
    }

    if (m_vertShaderModule == VK_NULL_HANDLE || m_fragShaderModule == VK_NULL_HANDLE) {
        qWarning("Shader modules not created!");
        return;
    }

    VkResult result;

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = m_vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Geometry::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Geometry::Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Geometry::Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = nullptr;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = sizeof(dynamicStates)/sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;

    // Pipeline layout (create if not exists)
    if (m_pipelineTriangleLayout == VK_NULL_HANDLE) {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(QMatrix4x4);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;

        result = _vkManager->vkCreatePipelineLayout(&pipelineLayoutInfo, nullptr, &m_pipelineTriangleLayout);
        if (result != VK_SUCCESS) {
            qWarning("Failed to create pipeline layout: %d", result);
            return;
        }
        qDebug("Pipeline layout created: %p", m_pipelineTriangleLayout);
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineTriangleLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = _vkManager->vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1,
                                                   &pipelineInfo, nullptr,
                                                   &m_graphicsTrianglePipeline);
    if (result != VK_SUCCESS) {
      qWarning("Failed to create graphics pipeline: %d", result);
      m_graphicsTrianglePipeline = VK_NULL_HANDLE;
      return;
    }

    m_trianglePipelineCreated = true;
    qDebug("Graphics triangle pipeline created successfully!");
}

void VulkanRenderNode::createLinePipeline(VkRenderPass renderPass)
{
    // Verify we have a valid render pass
    if (renderPass == VK_NULL_HANDLE) {
        qWarning("Cannot create pipeline: invalid render pass");
        return;
    }

    // Don't recreate if already created
    if (m_linePipelineCreated && m_graphicsLinePipeline != VK_NULL_HANDLE) {
        return;
    }

    qDebug("Creating graphics pipeline with render pass %p", renderPass);


    if (m_vertShaderModule == VK_NULL_HANDLE || m_fragShaderModule == VK_NULL_HANDLE) {
        qWarning("Shader modules not created!");
        return;
    }

    VkResult result;

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = m_vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_fragDashShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Geometry::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Geometry::Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Geometry::Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = nullptr;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = 3;
    dynamicState.pDynamicStates = dynamicStates;

    // Pipeline layout (create if not exists)
    if (m_pipelineLineLayout == VK_NULL_HANDLE) {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(QMatrix4x4);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;

        result = _vkManager->vkCreatePipelineLayout( &pipelineLayoutInfo, nullptr, &m_pipelineLineLayout);
        if (result != VK_SUCCESS) {
            qWarning("Failed to create pipeline layout: %d", result);
            return;
        }
        qDebug("Pipeline layout created: %p", m_pipelineTriangleLayout);
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = _vkManager->vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsLinePipeline);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create graphics pipeline: %d", result);
        m_graphicsLinePipeline = VK_NULL_HANDLE;
        return;
    }

    m_linePipelineCreated = true;
    qDebug("Graphics line pipeline created successfully!");
}

void VulkanRenderNode::createCirclePipeline(VkRenderPass renderPass)
{
    // Verify we have a valid render pass
    if (renderPass == VK_NULL_HANDLE) {
        qWarning("Cannot create pipeline: invalid render pass");
        return;
    }

    // Don't recreate if already created
    // if (m_linePipelineCreated && m_graphicsLinePipeline != VK_NULL_HANDLE) {
    //     return;
    // }

    qDebug("Creating graphics pipeline with render pass %p", renderPass);


    // if (m_vertShaderModule == VK_NULL_HANDLE || m_fragShaderModule == VK_NULL_HANDLE) {
    //     qWarning("Shader modules not created!");
    //     return;
    // }

    VkResult result;

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = m_vertCircleModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = m_fragCircleModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Geometry::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Geometry::Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Geometry::Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.pNext = nullptr;
    depthStencil.flags = 0;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = 3;
    dynamicState.pDynamicStates = dynamicStates;

    // Pipeline layout (create if not exists)
    if (m_pipelineLineLayout == VK_NULL_HANDLE) {
        VkPushConstantRange pushConstantRanges[2];
        pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRanges[0].offset = 0;
        pushConstantRanges[0].size = sizeof(float) * 2;

        pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRanges[1].offset = sizeof(float) * 2;
        pushConstantRanges[1].size = sizeof(float);


        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 2;
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;

        result = _vkManager->vkCreatePipelineLayout( &pipelineLayoutInfo, nullptr, &m_pipelineCircleLayout);
        if (result != VK_SUCCESS) {
            qWarning("Failed to create pipeline layout: %d", result);
            return;
        }
        qDebug("Pipeline layout created: %p", m_pipelineTriangleLayout);
    }

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = nullptr;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // qDebug("Calling vkCreateGraphicsPipelines...");
    // qDebug("  Device: %p", _vkManager->device());
    // qDebug("  Pipeline layout: %p", m_pipelineCircleLayout);
    // qDebug("  Render pass: %p", renderPass);
    // qDebug("  Vert shader: %p", m_vertCircleModule);
    // qDebug("  Frag shader: %p", m_fragCircleModule);

    result = _vkManager->vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsCirclePipeline);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create graphics pipeline: %d", result);
        m_graphicsCirclePipeline = VK_NULL_HANDLE;
        return;
    }

    m_circlePipelineCreated = true;
    qDebug("Graphics line pipeline created successfully!");
}

void VulkanRenderNode::updateVertexBuffer()
{
    bufferTriangle.updateMemory(0, m_verticesTriangle.data(), m_verticesTriangle.size() * sizeof(decltype(m_verticesTriangle)::value_type));
}

void VulkanRenderNode::updateVertexAddedLinesBuffer(const Geometry::Line& line, size_t index)
{
    // if (m_vertexAddedLinesBuffer == VK_NULL_HANDLE)
    //     return;

    // void* data;

    // VkResult result = _vkManager->devFuncs()->vkMapMemory(_vkManager->device(), m_vertexAddedLinesBufferMemory, (m_verticesAddedLines.size() - 1)*sizeof(m_verticesAddedLines[0]), 
    //                                          sizeof(Geometry::Line) * m_verticesAddedLines.size(), 0, &data);
    // if (result == VK_SUCCESS) {
    //     memcpy(data, &m_verticesAddedLines[m_verticesAddedLines.size() -1], sizeof(Geometry::Line));
    //     _vkManager->devFuncs()->vkUnmapMemory(_vkManager->device(), m_vertexAddedLinesBufferMemory);
    //     m_verticesAddedLinesDirty = false;
    // }
    decltype(m_verticesAddedLines)::value_type* data = new decltype(m_verticesAddedLines)::value_type;
    *data = m_verticesAddedLines.at(index);
    bufferAddedLines.updateMemory(index * sizeof(decltype(m_verticesAddedLines)::value_type), data, sizeof(decltype(m_verticesAddedLines)::value_type));
    m_verticesAddedLinesDirty = false;
}

void VulkanRenderNode::updateVertexPosition(const QPointF& position)
{
    // qDebug() << "Updating vertex position to:" << position;
    if (m_verticesTriangle.size() >= 1) {

        double x = (position.x() / _vkManager->item()->width()) * 2.0 - 1.0;
        double y = (position.y() / _vkManager->item()->height()) * 2.0 - 1.0;

        m_verticesTriangle[0].pos[0] = static_cast<float>(x);
        m_verticesTriangle[0].pos[1] = static_cast<float>(y);
        
        m_verticesDirty = true;
        // qDebug() << "Vertex updated to:" << m_verticesTriangle[0].pos[0] << m_verticesTriangle[0].pos[1];
    }
    updateVertexBuffer();
}

void VulkanRenderNode::render(const RenderState *state)
{
    if (!m_initialized || m_commandBuffer == VK_NULL_HANDLE)
        return;

    VkRenderPass currentRenderPass = *_vkManager->getResource<VkRenderPass>(QSGRendererInterface::RenderPassResource);

    if (currentRenderPass == VK_NULL_HANDLE) {
        qWarning("Invalid render pass at render time!");
        return;
    }

    // Create pipeline on first render with the actual render pass
    if (!m_trianglePipelineCreated) {
        createTrianglePipeline(currentRenderPass);
        createLinePipeline(currentRenderPass);
    }

    if (m_circlePipelineCreated) {
        createCirclePipeline(currentRenderPass);
    }

    if (m_graphicsTrianglePipeline == VK_NULL_HANDLE)
        return;

    recordCommandBuffer(state);
}

void VulkanRenderNode::recordCommandBuffer(const RenderState *state)
{

    VkCommandBuffer qtCommandBuffer = *_vkManager->getResource<VkCommandBuffer>(QSGRendererInterface::CommandListResource);

    if (qtCommandBuffer == VK_NULL_HANDLE) {
        qWarning("No command buffer from Qt!");
        return;
    }

    // Use Qt's command buffer instead of our own
    VkCommandBuffer commandBuffer = qtCommandBuffer;
    drawNet(commandBuffer);
    drawLine(commandBuffer);
    // drawTriangle(commandBuffer);
    drawAddedLines(commandBuffer);
}

void VulkanRenderNode::drawTriangle(VkCommandBuffer commandBuffer)
{
    QQuickWindow *window = _vkManager->item()->window();

    float time = QTime::currentTime().msecsSinceStartOfDay() % 10000;
    float angle = time;
    QMatrix4x4 model{};
    // model.translate({(float)(pos.x()/itemSize.width())/(z*20.f), (float)(pos.y()/itemSize.height())/(z*20.f), 0});
    model.rotate(angle/2.0f, 0, 1, 0);
    QMatrix4x4 view{};
    view.translate({0.0f, 0.0f, -4.0f});
    QMatrix4x4 projection{};
    projection.perspective(qDegreesToRadians(90), 1.0f, 1.0f, 5.0f);
    // projection.data()[1 + 1*4] *= -1;
    QMatrix4x4 mvp = projection * view * model;
    mvp.scale(VulkanRenderNode::z);

    vkCmdPushConstants(
        commandBuffer,
        m_pipelineTriangleLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        mvp.constData()
    );

    // Bind pipeline
    _vkManager->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsTrianglePipeline);

    // Set viewport and scissor
    QRectF rect = matrix()->mapRect(QRectF(0, 0, _vkManager->item()->width(), _vkManager->item()->height()));
    qreal dpr = window->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());
    _viewPort= rect;
    VkViewport viewport = {};
    viewport.x = rect.x() * dpr;
    viewport.y = rect.y() * dpr;
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    _vkManager->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
   _vkManager->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {bufferTriangle};
    VkDeviceSize offsets[] = {0};
    _vkManager->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // Draw - this is now valid because we're in Qt's render pass
    _vkManager->vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void VulkanRenderNode::drawNet(VkCommandBuffer commandBuffer)
{
    auto itemSize = _vkManager->item()->size();

    float time = QTime::currentTime().msecsSinceStartOfDay() % 10000;
    float angle = time;

    QMatrix4x4 mvp = {};
    static float localZ = 1;
    // static float localMax = 1.0f;
    // if (localZ < 0.3) {
    //     localMax = 1 / z;
    // }
    localZ = z;
    mvp.scale(localZ);
    mvp.translate((float)(pos.x()/itemSize.width())/localZ, (float)(pos.y()/itemSize.height())/localZ, 0);
    vkCmdPushConstants(
        commandBuffer,
        m_pipelineLineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        mvp.constData()
    );

    // Bind pipeline
    _vkManager->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsLinePipeline);

    // Set viewport and scissor
    QRectF rect = matrix()->mapRect(QRectF(0, 0, _vkManager->item()->width(), _vkManager->item()->height()));
    qreal dpr = _vkManager->itemWindow()->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());
    _viewPort= rect;
    VkViewport viewport = {};
    viewport.x = rect.x() * dpr;
    viewport.y = rect.y() * dpr;
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    _vkManager->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
   _vkManager->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    _vkManager->vkCmdSetLineWidth(commandBuffer, 1);

    VkBuffer vertexBuffers[] = {bufferNet};
    VkDeviceSize offsets[] = {0};
    _vkManager->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    _vkManager->vkCmdDraw(commandBuffer, m_verticesNet.size(), 1, 0, 0);
}

void VulkanRenderNode::drawAddedLines(VkCommandBuffer commandBuffer)
{
    auto itemSize = _vkManager->item()->size();

    QMatrix4x4 mvp = {};
    static float localZ = 1;
    // static float localMax = 1.0f;
    // if (localZ < 0.3) {
    //     localMax = 1 / z;
    // }
    localZ = z;
    mvp.scale(localZ);
    mvp.translate((float)(pos.x()/itemSize.width())/localZ, (float)(pos.y()/itemSize.height())/localZ, 0);
    vkCmdPushConstants(
        commandBuffer,
        m_pipelineLineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        mvp.constData()
    );

    // Bind pipeline
    _vkManager->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsLinePipeline);

    // Set viewport and scissor
    QRectF rect = matrix()->mapRect(QRectF(0, 0, _vkManager->item()->width(), _vkManager->item()->height()));
    qreal dpr = _vkManager->itemWindow()->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());
    _viewPort= rect;
    VkViewport viewport = {};
    viewport.x = rect.x() * dpr;
    viewport.y = rect.y() * dpr;
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    _vkManager->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
   _vkManager->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    _vkManager->vkCmdSetLineWidth(commandBuffer, 3);

    VkBuffer vertexBuffers[] = {bufferAddedLines};
    VkDeviceSize offsets[] = {0};
    _vkManager->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    _vkManager->vkCmdDraw(commandBuffer, m_verticesAddedLines.size() * 2, 1, 0, 0);
}

void VulkanRenderNode::drawLine(VkCommandBuffer commandBuffer)
{
    auto itemSize = _vkManager->item()->size();


    QMatrix4x4 i = {};
    i.translate((float)(pos.x()/itemSize.width()), (float)(pos.y()/itemSize.height()), 0);

    vkCmdPushConstants(
        commandBuffer,
        m_pipelineTriangleLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        i.constData()
    );

    _vkManager->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsLinePipeline);
    QRectF rect = matrix()->mapRect(QRectF(0, 0, _vkManager->item()->width(), _vkManager->item()->height()));
    qreal dpr = _vkManager->item()->window()->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());

    VkViewport viewport = {};
    viewport.x = rect.x() * dpr;
    viewport.y = rect.y() * dpr;
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    _vkManager->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
    _vkManager->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    _vkManager->vkCmdSetViewport(commandBuffer, 1, 1, &viewport);

    _vkManager->vkCmdSetScissor(commandBuffer, 1, 1, &scissor);

    VkBuffer vertexLineBuffers[] = {bufferLine};
    _vkManager->vkCmdSetLineWidth(commandBuffer, 5);
    VkDeviceSize offsets[] = {0};

    _vkManager->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexLineBuffers, offsets);

    _vkManager->vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void VulkanRenderNode::releaseResources()
{
    // if (_vkManager->device() == VK_NULL_HANDLE || !m_devFuncs)
        // return;

    if (m_graphicsTrianglePipeline != VK_NULL_HANDLE) {
        _vkManager->vkDestroyPipeline(m_graphicsTrianglePipeline, nullptr);
        m_graphicsTrianglePipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineTriangleLayout != VK_NULL_HANDLE) {
        _vkManager->vkDestroyPipelineLayout(m_pipelineTriangleLayout, nullptr);
        m_pipelineTriangleLayout = VK_NULL_HANDLE;
    }

    if (m_graphicsLinePipeline != VK_NULL_HANDLE) {
        _vkManager->vkDestroyPipeline(m_graphicsLinePipeline, nullptr);
        m_graphicsLinePipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLineLayout != VK_NULL_HANDLE) {
        _vkManager->vkDestroyPipelineLayout(m_pipelineLineLayout, nullptr);
        m_pipelineLineLayout = VK_NULL_HANDLE;
    }

    // if (m_vertShaderModule != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyShaderModule(_vkManager->device(), m_vertShaderModule, nullptr);
    //     m_vertShaderModule = VK_NULL_HANDLE;
    // }

    // if (m_fragShaderModule != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyShaderModule(_vkManager->device(), m_fragShaderModule, nullptr);
    //     m_fragShaderModule = VK_NULL_HANDLE;
    // }

    // if (m_fragDashShaderModule != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyShaderModule(_vkManager->device(), m_fragDashShaderModule, nullptr);
    //     m_fragDashShaderModule = VK_NULL_HANDLE;
    // }

    // if (m_vertCircleModule != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyShaderModule(_vkManager->device(), m_vertCircleModule, nullptr);
    // }

    // if (m_fragCircleModule != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyShaderModule(_vkManager->device(), m_fragCircleModule, nullptr);
    // }

    // if (m_vertexTriangleBuffer != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyBuffer(_vkManager->device(), m_vertexTriangleBuffer, nullptr);
    //     m_vertexTriangleBuffer = VK_NULL_HANDLE;
    // }

    // if (m_vertexTriangleBufferMemory != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkFreeMemory(_vkManager->device(), m_vertexTriangleBufferMemory, nullptr);
    //     m_vertexTriangleBufferMemory = VK_NULL_HANDLE;
    // }

    // if (m_vertexLineBuffer != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyBuffer(_vkManager->device(), m_vertexLineBuffer, nullptr);
    //     m_vertexLineBuffer = VK_NULL_HANDLE;
    // }

    // if (m_vertexLineBufferMemory != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkFreeMemory(_vkManager->device(), m_vertexLineBufferMemory, nullptr);
    //     m_vertexLineBufferMemory = VK_NULL_HANDLE;
    // }

    // if (m_vertexNetBuffer != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyBuffer(_vkManager->device(), m_vertexNetBuffer, nullptr);
    //     m_vertexNetBuffer = VK_NULL_HANDLE;
    // }

    // if (m_vertexNetBufferMemory != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkFreeMemory(_vkManager->device(), m_vertexNetBufferMemory, nullptr);
    //     m_vertexNetBufferMemory = VK_NULL_HANDLE;
    // }

    // if (m_vertexAddedLinesBuffer != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkDestroyBuffer(_vkManager->device(), m_vertexAddedLinesBuffer, nullptr);
    //     m_vertexAddedLinesBuffer = VK_NULL_HANDLE;
    // }

    // if (m_vertexAddedLinesBufferMemory != VK_NULL_HANDLE) {
    //     _vkManager->devFuncs()->vkFreeMemory(_vkManager->device(), m_vertexAddedLinesBufferMemory, nullptr);
    //     m_vertexAddedLinesBufferMemory = VK_NULL_HANDLE;
    // }

    if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE) {
        _vkManager->vkFreeCommandBuffers(m_commandPool, 1, &m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
    
    if (m_commandPool != VK_NULL_HANDLE) {
        _vkManager->vkDestroyCommandPool(m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    m_initialized = false;
}

QSGRenderNode::StateFlags VulkanRenderNode::changedStates() const
{
    return BlendState | ScissorState | ViewportState;
}

QSGRenderNode::RenderingFlags VulkanRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

void VulkanRenderNode::connectController(MainWindow* controller)
{
    m_verticesAddedLines = controller->lines;
    m_verticesAddedLines.subscribe([this](const Geometry::Line& line, size_t index) {
        updateVertexAddedLinesBuffer(line, index);
    });
}