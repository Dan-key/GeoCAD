#include "VulkanItem.h"
#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <QSGTexture>
#include <QTimer>
#include <vulkan/vulkan.h>

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(VulkanWindow *w) : m_window(w) {
        w->m_renderer = this;
    }

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

private:
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs = nullptr;
    bool m_initialized = false;
};

void VulkanRenderer::initResources()
{
    qDebug() << "VulkanRenderer::initResources()";
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
    m_initialized = true;
}

void VulkanRenderer::initSwapChainResources()
{
    qDebug() << "VulkanRenderer::initSwapChainResources()";
    // Initialize swapchain resources
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug() << "VulkanRenderer::releaseSwapChainResources()";
    // Cleanup swapchain resources
}

void VulkanRenderer::releaseResources()
{
    qDebug() << "VulkanRenderer::releaseResources()";
    m_initialized = false;
    // Cleanup resources
}

void VulkanRenderer::startNextFrame()
{
    if (!m_initialized || !m_devFuncs) {
        m_window->frameReady();
        return;
    }

    // Clear color: blue gradient
    VkClearColorValue clearColor = {{ 0.2f, 0.4f, 0.8f, 1.0f }};
    VkClearDepthStencilValue clearDS = { 1.0f, 0 };
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = m_window->defaultRenderPass();
    rpBeginInfo.framebuffer = m_window->currentFramebuffer();

    const QSize size = m_window->swapChainImageSize();
    rpBeginInfo.renderArea.extent.width = size.width();
    rpBeginInfo.renderArea.extent.height = size.height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;

    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Here you would add your Vulkan drawing commands
    // For now, we just clear to a blue color

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    // Create a QImage representation for Qt integration
    QImage frame(size, QImage::Format_RGBA8888);
    frame.fill(QColor(51, 102, 204)); // Same blue as clear color
    m_window->m_currentFrame = frame;

    m_window->frameReady();
}

// VulkanWindow implementation
VulkanWindow::VulkanWindow()
{
    // Set Vulkan window flags (not Qt window flags)
    setFlags(QVulkanWindow::PersistentResources);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    qDebug() << "VulkanWindow::createRenderer()";
    return new VulkanRenderer(this);
}

QImage VulkanWindow::getRenderedImage()
{
    return m_currentFrame;
}

void VulkanWindow::renderFrame()
{
    if (m_renderer) {
        requestUpdate();
    }
}

// VulkanItem implementation
VulkanItem::VulkanItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    qDebug() << "VulkanItem constructor, parent"  << parent;

    // Setup Vulkan instance
    m_instance.setApiVersion(QVersionNumber(1, 3, 275));

    // Enable validation layers in debug mode
#ifndef NDEBUG
    m_instance.setLayers(QByteArrayList() << "VK_LAYER_NV_optimus");
#endif

    if (!m_instance.create()) {
        qWarning("Failed to create Vulkan instance: error code %d", m_instance.errorCode());
        return;
    }

    qDebug() << "Vulkan instance created successfully";

    // Setup update timer for continuous rendering
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(16); // ~60 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &VulkanItem::updateTexture);

    // Connect to window changes
    connect(this, &QQuickItem::windowChanged, this, &VulkanItem::handleWindowChanged);
}

VulkanItem::~VulkanItem()
{
    qDebug() << "VulkanItem destructor";

    if (m_updateTimer) {
        m_updateTimer->stop();
    }

    if (m_vulkanWindow) {
        delete m_vulkanWindow;
        m_vulkanWindow = nullptr;
    }
}

void VulkanItem::setupVulkanWindow()
{
    if (m_vulkanWindow || !m_instance.isValid()) {
        return;
    }

    qDebug() << "Setting up Vulkan window";

    m_vulkanWindow = new VulkanWindow();
    m_vulkanWindow->setVulkanInstance(&m_instance);

    // Set size
    QSize size = boundingRect().size().toSize();
    if (size.isEmpty()) {
        size = QSize(512, 512);
    }

    qDebug() << "size:" << size;

    m_vulkanWindow->resize(size);

    // Create the window
    m_vulkanWindow->create();

    if (m_vulkanWindow->isValid()) {
        qDebug() << "Vulkan window created successfully";
        m_vulkanInitialized = true;
        m_updateTimer->start();
    } else {
        qWarning() << "Failed to create valid Vulkan window";
        delete m_vulkanWindow;
        m_vulkanWindow = nullptr;
    }
}

void VulkanItem::handleWindowChanged(QQuickWindow *win)
{
    if (win && !m_vulkanInitialized) {
        // Defer Vulkan setup until we have a Qt window context
        QTimer::singleShot(0, this, &VulkanItem::setupVulkanWindow);
    }
}

void VulkanItem::updateTexture()
{
    if (m_vulkanWindow && window()) {
        m_vulkanWindow->renderFrame();
        update(); // Trigger updatePaintNode
    }
}

QSGNode *VulkanItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);

    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>(node);

    if (!textureNode) {
        textureNode = new QSGSimpleTextureNode();
        textureNode->setFiltering(QSGTexture::Linear);
        textureNode->setRect(boundingRect());
        
        // Setup Vulkan if not done yet
        if (!m_vulkanInitialized && window()) {
            setupVulkanWindow();
        }
    }

    // Update texture if we have valid Vulkan rendering
    if (m_vulkanWindow && m_vulkanWindow->isValid() && window()) {
        QImage renderedImage = m_vulkanWindow->getRenderedImage();

        if (!renderedImage.isNull()) {
            QSGTexture *texture = window()->createTextureFromImage(renderedImage);
            if (texture) {
                // Clean up old texture
                QSGTexture *oldTexture = textureNode->texture();
                if (oldTexture && textureNode->ownsTexture()) {
                    delete oldTexture;
                }

                textureNode->setTexture(texture);
                textureNode->setOwnsTexture(true);
                textureNode->setRect(boundingRect());
            }
        }

        // Handle size changes
        QSize newSize = boundingRect().size().toSize();
        if (!newSize.isEmpty() && m_vulkanWindow->size() != newSize) {
            m_vulkanWindow->resize(newSize);
        }
    } else {
        // Fallback: create a simple animated texture
        static int frame = 0;
        frame++;

        QImage fallbackImage(256, 256, QImage::Format_RGBA8888);
        // Create a simple animation - changing colors
        int r = (frame * 2) % 255;
        int g = (frame * 3) % 255; 
        int b = (frame * 5) % 255;
        fallbackImage.fill(QColor(r, g, b));

        if (window()) {
            QSGTexture *fallbackTexture = window()->createTextureFromImage(fallbackImage);
            if (fallbackTexture) {
                // Clean up old texture
                QSGTexture *oldTexture = textureNode->texture();
                if (oldTexture && textureNode->ownsTexture()) {
                    delete oldTexture;
                }

                textureNode->setTexture(fallbackTexture);
                textureNode->setOwnsTexture(true);
                textureNode->setRect(boundingRect());
            }
        }
    }

    return textureNode;
}