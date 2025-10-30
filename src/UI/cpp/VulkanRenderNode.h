#pragma once

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>

#include "Geometry/Line.h"
#include "Library/Files/SpirvByteCode.h"

class VulkanRenderNode : public QSGRenderNode
{
public:
    VulkanRenderNode(QQuickItem *item);
    ~VulkanRenderNode();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;

    void updateVertexPosition(const QPointF& position);
    void addLine(const Geometry::Line& line);
    void updateLine(const Geometry::Line& line);

    static float z;
    static QPointF pos;
private:
    void initVulkan();
    void createCommandPool();
    void createTriangleVertexBuffer();
    void createLineVertexBuffer();
    void createNetVertexBuffer();
    void createAddedLinesVertexBuffer();

    void createShaderModules();

    void createTrianglePipeline(VkRenderPass renderPass);
    void createLinePipeline(VkRenderPass renderPass);
    void createCirclePipeline(VkRenderPass renderPass);

    void recordCommandBuffer(const RenderState *state);
    void updateVertexBuffer();
    void updateVertexAddedLinesBuffer();

    void drawTriangle(VkCommandBuffer);
    void drawLine(VkCommandBuffer);
    void drawNet(VkCommandBuffer);
    void drawAddedLines(VkCommandBuffer);

    QQuickItem *m_item;
    QVulkanInstance *m_vulkanInstance = nullptr;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    
    VkBuffer m_vertexTriangleBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexTriangleBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_vertexLineBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexLineBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_vertexNetBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexNetBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_vertexAddedLinesBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexAddedLinesBufferMemory = VK_NULL_HANDLE;
    
    VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragDashShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_vertCircleModule = VK_NULL_HANDLE;
    VkShaderModule m_fragCircleModule = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineTriangleLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsTrianglePipeline = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineLineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsLinePipeline = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineCircleLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsCirclePipeline = VK_NULL_HANDLE;

    bool m_initialized = false;
    bool m_trianglePipelineCreated = false;
    bool m_linePipelineCreated = false;
    bool m_circlePipelineCreated = false;

    // Store vertices for dynamic updates
    std::vector<Geometry::Vertex> m_verticesTriangle;
    std::vector<Geometry::Vertex> m_verticesLine;
    std::vector<Geometry::Vertex> m_verticesNet;
    std::vector<Geometry::Line> m_verticesAddedLines;

    QRectF _viewPort {};

    bool m_verticesDirty = false;
    bool m_verticesAddedLinesDirty = true;

    Files::SpirvByteCode vertShaderCode;
    Files::SpirvByteCode fragShaderCode;
    Files::SpirvByteCode fragDashShaderCode;
    Files::SpirvByteCode fragCircleShaderCode;
    Files::SpirvByteCode vertCircleShaderCode;
};