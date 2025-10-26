#ifndef VULKANITEM_H
#define VULKANITEM_H

#include <QQuickItem>
#include <QSGRenderNode>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QVulkanDeviceFunctions>

struct Vertex {
    float pos[2];
    float color[3];
};

class VulkanRenderNode : public QSGRenderNode
{
public:
    VulkanRenderNode(QQuickItem *item);
    ~VulkanRenderNode();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;

    // Add method to update vertices based on mouse input
    void updateVertexPosition(const QPointF& position);

private:
    void initVulkan();
    void createCommandPool();
    void createTriangleVertexBuffer();
    void createLineVertexBuffer();
    void createShaderModules();
    void createTrianglePipeline(VkRenderPass renderPass);
    void createLinePipeline(VkRenderPass renderPass);
    void recordCommandBuffer(const RenderState *state);
    void updateVertexBuffer();

    void drawTriangle(VkCommandBuffer);
    void drawLine(VkCommandBuffer);

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
    
    VkShaderModule m_vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragShaderModule = VK_NULL_HANDLE;
    VkShaderModule m_fragDashShaderModule = VK_NULL_HANDLE;
    
    VkPipelineLayout m_pipelineTriangleLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsTrianglePipeline = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineLineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsLinePipeline = VK_NULL_HANDLE;
    
    bool m_initialized = false;
    bool m_trianglePipelineCreated = false;
    bool m_linePipelineCreated = false;

    
    // Store vertices for dynamic updates
    std::vector<Vertex> m_verticesTriangle;
    std::vector<Vertex> m_verticesLine;

    bool m_verticesDirty = false;
};

class VulkanItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    // Add properties for mouse interaction
    // Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    // Q_PROPERTY(QColor triangleColor READ triangleColor WRITE setTriangleColor NOTIFY triangleColorChanged)

public:
    VulkanItem(QQuickItem *parent = nullptr);

    // bool interactive() const { return m_interactive; }
    // void setInteractive(bool interactive);

    // QColor triangleColor() const { return m_triangleColor; }
    // void setTriangleColor(const QColor& color);
    static float z;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    
    // Mouse event handlers
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void interactiveChanged();
    void triangleColorChanged();
    void mousePressed(qreal x, qreal y);
    void mouseMoved(qreal x, qreal y);
    void mouseReleased(qreal x, qreal y);
    void hoverPositionChanged(qreal x, qreal y);

private:
    VulkanRenderNode *m_renderNode = nullptr;
    // bool m_interactive = true;
    // QColor m_triangleColor = Qt::red;
    bool m_mousePressed = false;
};

#endif // VULKANITEM_H