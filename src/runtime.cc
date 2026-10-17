/*
 * runtime.h 구현. PassContext의 실제 구현(GpuRenderer::Context)도 여기 있다.
 * - 구현할 것: 프레임 슬롯별 텍스처, fence 기반 지연 해제, GPU timestamp, 파이프라인·dispatch·draw.
 */
#include "runtime.h"

#include <cstdio>
#include <vector>

namespace RD {

class GpuRenderer::Context : public PassContext {
public:
    explicit Context(VulkanDevice &device) : _device(device) {}
    ~Context() override { releaseAll(); }

    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }

    Texture createTexture(Format format, uint32_t width, uint32_t height) override {
        GpuImage *image = _device.createImage(format, width, height);
        if(image != nullptr)
            _owned.push_back(image);
        return Texture{image};
    }

    void clear(Texture target, float r, float g, float b, float a) override {
        if(_cmd != VK_NULL_HANDLE && target.image != nullptr)
            clearImage(_cmd, *target.image, r, g, b, a);
    }

    void blit(Texture src, Texture dst) override {
        if(dst.image == nullptr)
            return;
        blitTo(src, dst, Rect{0, 0, dst.image->extent.width, dst.image->extent.height});
    }

    void blitTo(Texture src, Texture dst, Rect dstRect) override {
        if(_cmd != VK_NULL_HANDLE && src.image != nullptr && dst.image != nullptr)
            blitImage(_cmd, *src.image, *dst.image, dstRect);
    }

    void releaseAll() {
        for(GpuImage *image : _owned)
            _device.destroyImage(image);
        _owned.clear();
    }

    void setExtent(uint32_t width, uint32_t height) {
        _width = width;
        _height = height;
    }
    void setCommandBuffer(VkCommandBuffer cmd) { _cmd = cmd; }

private:
    VulkanDevice &_device;
    VkCommandBuffer _cmd = VK_NULL_HANDLE;
    uint32_t _width = 0;
    uint32_t _height = 0;
    std::vector<GpuImage *> _owned;  // 지금 그래프가 prepare에서 만든 텍스처
};

// BGRA 또는 RGBA 8비트 픽셀을 24비트 BMP(아래에서 위로, 행 4바이트 정렬)로 저장한다.
static bool writeBmp(const std::string &path, const uint8_t *pixels, uint32_t width, uint32_t height, bool bgra) {
    FILE *file = std::fopen(path.c_str(), "wb");
    if(file == nullptr)
        return false;
    uint32_t rowSize = (width * 3 + 3) & ~3u;
    uint32_t dataSize = rowSize * height;
    uint8_t header[54] = {'B', 'M'};
    auto put32 = [&](int offset, uint32_t v) {
        for(int i = 0; i < 4; ++i)
            header[offset + i] = static_cast<uint8_t>(v >> (8 * i));
    };
    put32(2, 54 + dataSize);
    put32(10, 54);
    put32(14, 40);
    put32(18, width);
    put32(22, height);
    header[26] = 1;
    header[28] = 24;
    put32(34, dataSize);
    std::fwrite(header, 1, sizeof(header), file);

    std::vector<uint8_t> row(rowSize, 0);
    for(uint32_t y = 0; y < height; ++y) {
        const uint8_t *src = pixels + static_cast<size_t>(height - 1 - y) * width * 4;
        for(uint32_t x = 0; x < width; ++x) {
            row[x * 3 + 0] = bgra ? src[x * 4 + 0] : src[x * 4 + 2];
            row[x * 3 + 1] = src[x * 4 + 1];
            row[x * 3 + 2] = bgra ? src[x * 4 + 2] : src[x * 4 + 0];
        }
        std::fwrite(row.data(), 1, rowSize, file);
    }
    std::fclose(file);
    return true;
}

GpuRenderer::GpuRenderer() = default;

GpuRenderer::~GpuRenderer() {
    shutdown();
}

bool GpuRenderer::initialize(void *hinstance, void *hwnd, uint32_t width, uint32_t height, bool vsync,
                             bool validation, std::string &error) {
    _width = width;
    _height = height;
    if(!_device.initialize(hinstance, hwnd, validation, error))
        return false;
    if(!_swapchain.create(_device, width, height, vsync, error))
        return false;
    if(!_frames.create(_device, _swapchain, 2, error))
        return false;
    _context = std::make_unique<Context>(_device);
    _initialized = true;
    return true;
}

void GpuRenderer::shutdown() {
    if(!_initialized)
        return;
    _device.waitIdle();
    _context.reset();
    _frames.destroy();
    _swapchain.destroy();
    _device.shutdown();
    _initialized = false;
}

bool GpuRenderer::prepare(Graph &graph, std::string &error) {
    VkExtent2D extent = _swapchain.extent();
    _context->setExtent(extent.width, extent.height);
    _context->setCommandBuffer(VK_NULL_HANDLE);
    graph.prepare(*_context);
    (void)error;
    return true;
}

void GpuRenderer::releaseGraph() {
    _device.waitIdle();
    _context->releaseAll();
}

void GpuRenderer::resize(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    _resizePending = true;
}

bool GpuRenderer::renderFrame(Graph &graph, std::string &error) {
    if(_resizePending) {
        if(_width == 0 || _height == 0)
            return true;  // 최소화: 그리지 않는다.
        _device.waitIdle();
        std::string reason;
        if(!_swapchain.recreate(_width, _height, reason))
            return true;  // 아직 크기 0 등. 다음 프레임에 다시.
        if(!_frames.onSwapchainRecreated(error))
            return false;
        _resizePending = false;
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    GpuImage *target = nullptr;
    bool outOfDate = false;
    if(!_frames.beginFrame(cmd, target, outOfDate)) {
        if(outOfDate) {
            _resizePending = true;
            return true;
        }
        error = "vkAcquireNextImageKHR failed";
        return false;
    }

    _presentTarget.tex = Texture{target};
    graph.bindInput(kPresentTarget, &_presentTarget);

    _context->setCommandBuffer(cmd);
    bool ok = graph.inputsBound();
    if(ok)
        graph.record(*_context);
    else
        error = "graph inputs are not bound";

    // 캡처: 스왑체인 이미지를 버퍼로 복사하고, 호스트가 읽을 수 있게 장벽을 둔다.
    GpuBuffer readback;
    VkExtent2D extent = target->extent;
    if(!_capturePath.empty() && _swapchain.canReadBack()) {
        readback = _device.createReadbackBuffer(static_cast<VkDeviceSize>(extent.width) * extent.height * 4);
        if(readback.buffer != VK_NULL_HANDLE) {
            transition(cmd, *target, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                       VK_ACCESS_2_TRANSFER_READ_BIT);
            VkBufferImageCopy region{};
            region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
            region.imageExtent = {extent.width, extent.height, 1};
            vkCmdCopyImageToBuffer(cmd, target->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1,
                                   &region);
            VkMemoryBarrier2 toHost{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
            toHost.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            toHost.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            toHost.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
            toHost.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
            VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dep.memoryBarrierCount = 1;
            dep.pMemoryBarriers = &toHost;
            vkCmdPipelineBarrier2(cmd, &dep);
        }
    }

    transition(cmd, *target, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
               VK_ACCESS_2_NONE);
    _context->setCommandBuffer(VK_NULL_HANDLE);

    if(!_frames.endFrame(outOfDate)) {
        error = "vkQueueSubmit2 / present failed";
        return false;
    }
    if(outOfDate)
        _resizePending = true;

    if(readback.buffer != VK_NULL_HANDLE) {
        _device.waitIdle();
        void *mapped = nullptr;
        vkMapMemory(_device.device(), readback.memory, 0, readback.size, 0, &mapped);
        VkFormat format = _swapchain.format();
        bool bgra = format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_B8G8R8A8_UNORM;
        bool saved = writeBmp(_capturePath, static_cast<const uint8_t *>(mapped), extent.width, extent.height, bgra);
        vkUnmapMemory(_device.device(), readback.memory);
        _device.destroyBuffer(readback);
        std::printf("capture %s: %s\n", saved ? "saved" : "failed", _capturePath.c_str());
        _capturePath.clear();
    }
    return ok;
}

}
