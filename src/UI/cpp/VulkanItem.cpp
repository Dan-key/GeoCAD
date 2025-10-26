#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vulkan/vulkan.h>
#include "VulkanItem.h"
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <cstring>
#include <vector>
#include <iostream>
#include <QTimer>

namespace {

uint32_t* read_spv(const char* path, size_t* out_size_bytes) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    uint32_t* data = (uint32_t*)malloc(sz);
    if (!data) { fclose(f); return NULL; }
    if (fread(data, 1, sz, f) != (size_t)sz) { free(data); fclose(f); return NULL; }
    fclose(f);
    *out_size_bytes = (size_t)sz;
    return data;
}

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

float VulkanItem::z = 1.0f;

static uint32_t* vertShaderCode;
static size_t vertShaderSize;

static uint32_t* fragShaderCode;
static size_t fragShaderSize;

static uint32_t* fragDashShaderCode;
static size_t fragDashShaderSize;

VulkanRenderNode::VulkanRenderNode(QQuickItem *item)
    : m_item(item)
{
    size_t size = 0;
    vertShaderCode = read_spv("shaders/vertex.spv", &size);
    vertShaderSize = size;
    std::cout << "Vertex shader size: " << vertShaderSize << " bytes\n";
    fragShaderCode = read_spv("shaders/frag.spv", &size);
    fragShaderSize = size;
    std::cout << "Fragment shader size: " << fragShaderSize << " bytes\n";
    fragDashShaderCode = read_spv("shaders/frag_dash_line.spv", &size);
    fragDashShaderSize = size;
    std::cout << "Fragment dash shader size: " << fragShaderSize << " bytes\n";
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

void VulkanRenderNode::initVulkan()
{
    if (m_initialized)
        return;

    QQuickWindow *window = m_item->window();
    if (!window) {
        qWarning("No window available!");
        return;
    }

    QSGRendererInterface *rif = window->rendererInterface();
    if (!rif) {
        qWarning("No renderer interface!");
        return;
    }
 
    if (rif->graphicsApi() != QSGRendererInterface::VulkanRhi) {
        qWarning("Not using Vulkan RHI!");
        return;
    }

    m_vulkanInstance = window->vulkanInstance();
    if (!m_vulkanInstance) {
        qWarning("No Vulkan instance!");
        return;
    }

    m_device = *static_cast<VkDevice *>(rif->getResource(window, QSGRendererInterface::DeviceResource));
    m_physicalDevice = *static_cast<VkPhysicalDevice *>(rif->getResource(window, QSGRendererInterface::PhysicalDeviceResource));
    
    if (m_device == VK_NULL_HANDLE || m_physicalDevice == VK_NULL_HANDLE) {
        qWarning("Invalid Vulkan device or physical device!");
        return;
    }

    m_devFuncs = m_vulkanInstance->deviceFunctions(m_device);
    if (!m_devFuncs) {
        qWarning("Failed to get device functions!");
        return;
    }

    qDebug("Device functions pointer: %p", m_devFuncs);
    qDebug("Testing a simple Vulkan call...");

    // Test that device functions work with a simple call
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    qDebug("GPU: %s", props.deviceName);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &features);
    qDebug("wideline supported: %d", features.wideLines);

    createCommandPool();
    if (m_commandPool == VK_NULL_HANDLE || m_commandBuffer == VK_NULL_HANDLE) {
        qWarning("Failed to create command pool or buffer!");
        return;
    }

    createTriangleVertexBuffer();
    if (m_vertexTriangleBuffer == VK_NULL_HANDLE) {
        qWarning("Failed to create triangle vertex buffer!");
        return;
    }

    createLineVertexBuffer();
    if (m_vertexLineBuffer == VK_NULL_HANDLE) {
        qWarning("Failed to create line vertex buffer!");
        return;
    }

    createShaderModules();
    if (m_vertShaderModule == VK_NULL_HANDLE || m_fragShaderModule == VK_NULL_HANDLE) {
        qWarning("Failed to create shader modules!");
        return;
    }

    qDebug("Vulkan initialization successful!");
    m_initialized = true;
}

void VulkanRenderNode::createCommandPool()
{
    // Find graphics queue family
    uint32_t queueFamilyIndex = findGraphicsQueueFamily(m_physicalDevice);

    // Get the graphics queue
    // m_devFuncs->vkGetDeviceQueue(m_device, queueFamilyIndex, 0, &m_graphicsQueue);

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = m_devFuncs->vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
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

    result = m_devFuncs->vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
    if (result != VK_SUCCESS) {
        qWarning("Failed to allocate command buffer: %d", result);
    }
}

void VulkanRenderNode::createLineVertexBuffer()
{
    m_verticesLine = {
        {{0.0f, -1.0f}, {0.2f, 0.2f, 0.7f}},
        {{0.0f, 1.0f}, {0.2f, 0.2f, 0.7f}}
    };

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_verticesLine.size() * sizeof(decltype(m_verticesLine)::value_type);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_devFuncs->vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexLineBuffer);

    VkMemoryRequirements memRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_vertexLineBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_devFuncs->vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexLineBufferMemory);
    m_devFuncs->vkBindBufferMemory(m_device, m_vertexLineBuffer, m_vertexLineBufferMemory, 0);

    void* data;
    m_devFuncs->vkMapMemory(m_device, m_vertexLineBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, m_verticesLine.data() , m_verticesLine.size() * sizeof(decltype(m_verticesLine)::value_type));
    m_devFuncs->vkUnmapMemory(m_device, m_vertexLineBufferMemory);
}

void VulkanRenderNode::createTriangleVertexBuffer()
{
    // Triangle vertices with colors (position XY, color RGB)
    m_verticesTriangle = {
        {{0.0f, -0.05f}, {1.0f, 0.0f, 0.0f}},  // Top - Red
        {{0.05f, 0.05f}, {0.0f, 1.0f, 0.0f}},   // Right - Green
        {{-0.05f, 0.05f}, {0.0f, 0.0f, 1.0f}}   // Left - Blue
    };


    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_verticesTriangle.size() * sizeof(decltype(m_verticesTriangle)::value_type);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_devFuncs->vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexTriangleBuffer);

    VkMemoryRequirements memRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_vertexTriangleBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_devFuncs->vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexTriangleBufferMemory);
    m_devFuncs->vkBindBufferMemory(m_device, m_vertexTriangleBuffer, m_vertexTriangleBufferMemory, 0);

    void* data;
    m_devFuncs->vkMapMemory(m_device, m_vertexTriangleBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, m_verticesTriangle.data() , m_verticesTriangle.size() * sizeof(decltype(m_verticesTriangle)::value_type));
    m_devFuncs->vkUnmapMemory(m_device, m_vertexTriangleBufferMemory);
}

void VulkanRenderNode::createShaderModules()
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    // Vertex shader
    createInfo.codeSize = vertShaderSize;
    createInfo.pCode = vertShaderCode;

    qDebug("Creating vertex shader module, size: %zu bytes", vertShaderSize);
    VkResult result = m_devFuncs->vkCreateShaderModule(m_device, &createInfo, nullptr, &m_vertShaderModule);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create vertex shader module: %d", result);
        return;
    }
    qDebug("Vertex shader module created: %p", m_vertShaderModule);

    // Fragment shader
    createInfo.codeSize = fragShaderSize;
    createInfo.pCode = fragShaderCode;

    qDebug("Creating fragment shader module, size: %zu bytes", fragShaderSize);
    result = m_devFuncs->vkCreateShaderModule(m_device, &createInfo, nullptr, &m_fragShaderModule);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create fragment shader module: %d", result);
        return;
    }
    qDebug("Fragment shader module created: %p", m_fragShaderModule);

    createInfo.codeSize = fragDashShaderSize;
    createInfo.pCode = fragDashShaderCode;

    qDebug("Creating fragment dash shader module, size: %zu bytes", fragDashShaderSize);
    result = m_devFuncs->vkCreateShaderModule(m_device, &createInfo, nullptr, &m_fragDashShaderModule);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create fragment dashshader module: %d", result);
        return;
    }
    qDebug("Fragment dash shader module created: %p", m_fragDashShaderModule);
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
    if (!m_devFuncs) {
        qWarning("No device functions!");
        return;
    }

    if (m_device == VK_NULL_HANDLE) {
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
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

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

        result = m_devFuncs->vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineTriangleLayout);
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

    qDebug("Calling vkCreateGraphicsPipelines...");
    qDebug("  Device: %p", m_device);
    qDebug("  Pipeline layout: %p", m_pipelineTriangleLayout);
    qDebug("  Render pass: %p", renderPass);
    qDebug("  Vert shader: %p", m_vertShaderModule);
    qDebug("  Frag shader: %p", m_fragShaderModule);

    result = m_devFuncs->vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
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

    // Verify all prerequisites
    if (!m_devFuncs) {
        qWarning("No device functions!");
        return;
    }

    if (m_device == VK_NULL_HANDLE) {
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
    shaderStages[1].module = m_fragDashShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

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

        result = m_devFuncs->vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLineLayout);
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

    qDebug("Calling vkCreateGraphicsPipelines...");
    qDebug("  Device: %p", m_device);
    qDebug("  Pipeline layout: %p", m_pipelineTriangleLayout);
    qDebug("  Render pass: %p", renderPass);
    qDebug("  Vert shader: %p", m_vertShaderModule);
    qDebug("  Frag shader: %p", m_fragShaderModule);

    result = m_devFuncs->vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsLinePipeline);
    if (result != VK_SUCCESS) {
        qWarning("Failed to create graphics pipeline: %d", result);
        m_graphicsLinePipeline = VK_NULL_HANDLE;
        return;
    }

    m_linePipelineCreated = true;
    qDebug("Graphics line pipeline created successfully!");
}

void VulkanRenderNode::updateVertexBuffer()
{
    if (!m_verticesDirty || m_vertexTriangleBuffer == VK_NULL_HANDLE)
        return;

    void* data;
    VkResult result = m_devFuncs->vkMapMemory(m_device, m_vertexTriangleBufferMemory, 0, 
                                             sizeof(Vertex) * m_verticesTriangle.size(), 0, &data);
    if (result == VK_SUCCESS) {
        memcpy(data, m_verticesTriangle.data(), sizeof(Vertex) * m_verticesTriangle.size());
        m_devFuncs->vkUnmapMemory(m_device, m_vertexTriangleBufferMemory);
        m_verticesDirty = false;
    }
}

void VulkanRenderNode::updateVertexPosition(const QPointF& position)
{
    qDebug() << "Updating vertex position to:" << position;
    if (m_verticesTriangle.size() >= 1) {

        double x = (position.x() / m_item->width()) * 2.0 - 1.0;
        double y = (position.y() / m_item->height()) * 2.0 - 1.0;

        m_verticesTriangle[0].pos[0] = static_cast<float>(x);
        m_verticesTriangle[0].pos[1] = static_cast<float>(y);
        
        m_verticesDirty = true;
        qDebug() << "Vertex updated to:" << m_verticesTriangle[0].pos[0] << m_verticesTriangle[0].pos[1];
    }
    updateVertexBuffer();
}

void VulkanItem::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;
    QPointF localPos = event->position();

    qDebug() << "Mouse pressed at:" << localPos;
    emit mousePressed(localPos.x(), localPos.y());

    if (m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    }

    event->accept();
}

void VulkanItem::mouseMoveEvent(QMouseEvent *event)
{
    QPointF localPos = event->position();

    if (m_mousePressed && m_renderNode && event->buttons() & Qt::LeftButton) {
        m_renderNode->updateVertexPosition(localPos);
        update();
    }

    emit mouseMoved(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePressed = false;
    QPointF localPos = event->position();

    qDebug() << "Mouse released at:" << localPos;
    emit mouseReleased(localPos.x(), localPos.y());

    event->accept();
}

void VulkanItem::hoverEnterEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    emit hoverPositionChanged(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::hoverMoveEvent(QHoverEvent *event)
{
    QPointF localPos = event->position();
    emit hoverPositionChanged(localPos.x(), localPos.y());
    event->accept();
}

void VulkanItem::hoverLeaveEvent(QHoverEvent *event)
{
    emit hoverPositionChanged(-1, -1); // Signal that hover left
    event->accept();
}

void VulkanItem::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().ry();
    z = delta > 0 ? z / 0.9 : z * 0.9;
    qDebug() << "VulkanItem::z" << VulkanItem::z;
    event->accept();
}

void VulkanRenderNode::render(const RenderState *state)
{
    if (!m_initialized)
        initVulkan();

    if (!m_initialized || m_commandBuffer == VK_NULL_HANDLE)
        return;

    // Get the current render pass from Qt at render time
    QQuickWindow *window = m_item->window();
    if (!window)
        return;

    QSGRendererInterface *rif = window->rendererInterface();
    if (!rif)
        return;

    VkRenderPass currentRenderPass = *static_cast<VkRenderPass *>(
        rif->getResource(window, QSGRendererInterface::RenderPassResource));

    if (currentRenderPass == VK_NULL_HANDLE) {
        qWarning("Invalid render pass at render time!");
        return;
    }

    // Create pipeline on first render with the actual render pass
    if (!m_trianglePipelineCreated) {
        createTrianglePipeline(currentRenderPass);
        createLinePipeline(currentRenderPass);
    }

    if (m_graphicsTrianglePipeline == VK_NULL_HANDLE)
        return;

    recordCommandBuffer(state);
}

void VulkanRenderNode::recordCommandBuffer(const RenderState *state)
{
    // Get Qt's current command buffer
    QQuickWindow *window = m_item->window();
    if (!window)
        return;

    QSGRendererInterface *rif = window->rendererInterface();
    if (!rif)
        return;

    VkCommandBuffer qtCommandBuffer = *static_cast<VkCommandBuffer *>(
        rif->getResource(window, QSGRendererInterface::CommandListResource));

    if (qtCommandBuffer == VK_NULL_HANDLE) {
        qWarning("No command buffer from Qt!");
        return;
    }

    // Use Qt's command buffer instead of our own
    VkCommandBuffer commandBuffer = qtCommandBuffer;

    drawLine(commandBuffer);
    drawTriangle(commandBuffer);
}

void VulkanRenderNode::drawTriangle(VkCommandBuffer commandBuffer)
{
    QQuickWindow *window = m_item->window();

    float time = QTime::currentTime().msecsSinceStartOfDay() % 10000;
    float angle = time;
    QMatrix4x4 model{};
    model.rotate(angle/2.0f, 0, 1, 0);
    QMatrix4x4 view{};
    view.translate({0, 0 , -4.0f});
    QMatrix4x4 projection{};
    projection.perspective(qDegreesToRadians(90), 1.0f, 1.0f, 5.0f);
    // projection.data()[1 + 1*4] *= -1;
    QMatrix4x4 mvp = projection * view * model;
    mvp.scale(VulkanItem::z);
    vkCmdPushConstants(
        commandBuffer,
        m_pipelineTriangleLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        mvp.constData()
    );

    // Bind pipeline
    m_devFuncs->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsTrianglePipeline);

    // Set viewport and scissor
    QRectF rect = matrix()->mapRect(QRectF(m_item->x(), m_item->y(), m_item->width(), m_item->height()));
    qreal dpr = window->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());

    VkViewport viewport = {};
    viewport.x = rect.x();
    viewport.y = rect.y();
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_devFuncs->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
    m_devFuncs->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {m_vertexTriangleBuffer};
    VkDeviceSize offsets[] = {0};
    m_devFuncs->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // Draw - this is now valid because we're in Qt's render pass
    m_devFuncs->vkCmdDraw(commandBuffer, 3, 1, 0, 0);

}

void VulkanRenderNode::drawLine(VkCommandBuffer commandBuffer)
{
    QQuickWindow *window = m_item->window();

    QMatrix4x4 i = {};
    vkCmdPushConstants(
        commandBuffer,
        m_pipelineTriangleLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(QMatrix4x4),
        i.constData()   // pointer to raw float[16]
    );

    // Bind pipeline
    m_devFuncs->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsLinePipeline);

    // Set viewport and scissor
    QRectF rect = matrix()->mapRect(QRectF(m_item->x(), m_item->y(), m_item->width(), m_item->height()));
    qreal dpr = window->devicePixelRatio();
    rect.setWidth(dpr*rect.width());
    rect.setHeight(dpr*rect.height());

    VkViewport viewport = {};
    viewport.x = rect.x();
    viewport.y = rect.y();
    viewport.width = rect.width();
    viewport.height = rect.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_devFuncs->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {(int32_t)rect.x(), (int32_t)rect.y()};
    scissor.extent = {(uint32_t)rect.width(), (uint32_t)rect.height()};
    m_devFuncs->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    m_devFuncs->vkCmdSetViewport(commandBuffer, 1, 1, &viewport);

    m_devFuncs->vkCmdSetScissor(commandBuffer, 1, 1, &scissor);

    // Bind vertex buffer
    VkBuffer vertexLineBuffers[] = {m_vertexLineBuffer};
    m_devFuncs->vkCmdSetLineWidth(commandBuffer, 2);
    VkDeviceSize offsets[] = {0};

    m_devFuncs->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexLineBuffers, offsets);

    // Draw - this is now valid because we're in Qt's render pass
    m_devFuncs->vkCmdDraw(commandBuffer, 2, 1, 0, 0);
}

void VulkanRenderNode::releaseResources()
{
    if (m_device == VK_NULL_HANDLE || !m_devFuncs)
        return;

    if (m_graphicsTrianglePipeline != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipeline(m_device, m_graphicsTrianglePipeline, nullptr);
        m_graphicsTrianglePipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineTriangleLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineLayout(m_device, m_pipelineTriangleLayout, nullptr);
        m_pipelineTriangleLayout = VK_NULL_HANDLE;
    }

    if (m_graphicsLinePipeline != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipeline(m_device, m_graphicsLinePipeline, nullptr);
        m_graphicsLinePipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLineLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineLayout(m_device, m_pipelineLineLayout, nullptr);
        m_pipelineLineLayout = VK_NULL_HANDLE;
    }

    if (m_vertShaderModule != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_vertShaderModule, nullptr);
        m_vertShaderModule = VK_NULL_HANDLE;
    }

    if (m_fragShaderModule != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_fragShaderModule, nullptr);
        m_fragShaderModule = VK_NULL_HANDLE;
    }

    if (m_fragDashShaderModule != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_fragDashShaderModule, nullptr);
        m_fragDashShaderModule = VK_NULL_HANDLE;
    }

    if (m_vertexTriangleBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_vertexTriangleBuffer, nullptr);
        m_vertexTriangleBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexTriangleBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_vertexTriangleBufferMemory, nullptr);
        m_vertexTriangleBufferMemory = VK_NULL_HANDLE;
    }

    if (m_vertexLineBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_vertexLineBuffer, nullptr);
        m_vertexLineBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexLineBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_vertexLineBufferMemory, nullptr);
        m_vertexLineBufferMemory = VK_NULL_HANDLE;
    }

    if (m_commandBuffer != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
    
    if (m_commandPool != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyCommandPool(m_device, m_commandPool, nullptr);
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

// VulkanItem Implementation
VulkanItem::VulkanItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VulkanItem::update);
    timer->start(10); 
}

QSGNode *VulkanItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    VulkanRenderNode *node = static_cast<VulkanRenderNode *>(oldNode);
    if (!node)
        node = new VulkanRenderNode(this);
    m_renderNode = node;
    node->markDirty(QSGNode::DirtyMaterial);

    return node;
}