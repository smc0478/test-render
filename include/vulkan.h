/*
 * Vulkan 백엔드. Vk* 타입은 vulkan.h / vulkan.cc / runtime 밖으로 내보내지 않는다.
 * - GpuImage: VkImage + 메모리 + 현재 동기화 상태(layout, stage, access). 장벽은 이 상태를 보고 넣는다.
 * - VulkanDevice: volk로 vulkan-1.dll 로드, 인스턴스, Win32 surface, 물리·논리 장치(1.3, synchronization2), 이미지 생성.
 * - VulkanSwapchain: 생성·재생성(resize), 스왑체인 이미지를 GpuImage로 감싼다(소유하지 않음).
 * - FrameScheduler: 프레임 슬롯(명령 버퍼, fence, 세마포어). 제출과 present는 여기서만 한다.
 * - transition / clearImage / blitImage: Synchronization2 장벽과 transfer 명령.
 * - 필요: 이미지 뷰·파이프라인(셰이더 단계), 메모리 할당기(VMA), 검증 레이어 메시지 출력.
 * - 범위: 엔진 내부.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <volk.h>

#include "pass-context.h"

namespace RD {

struct GpuImage {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkExtent2D extent{};
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 access = VK_ACCESS_2_NONE;
    bool owned = true;  // 스왑체인 이미지는 false
};

// CPU가 읽는 버퍼(프레임 캡처용). HOST_VISIBLE | HOST_COHERENT.
struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

VkFormat toVkFormat(Format format);

class VulkanDevice {
public:
    bool initialize(void *hinstance, void *hwnd, bool validation, std::string &error);
    void shutdown();
    void waitIdle();

    VkPhysicalDevice physicalDevice() const { return _physical; }
    VkDevice device() const { return _device; }
    VkQueue queue() const { return _queue; }
    uint32_t queueFamily() const { return _queueFamily; }
    VkSurfaceKHR surface() const { return _surface; }
    const std::string &name() const { return _name; }

    GpuImage *createImage(Format format, uint32_t width, uint32_t height);
    void destroyImage(GpuImage *image);

    GpuBuffer createReadbackBuffer(VkDeviceSize size);
    void destroyBuffer(GpuBuffer &buffer);

private:
    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) const;

    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkPhysicalDevice _physical = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _queue = VK_NULL_HANDLE;
    uint32_t _queueFamily = 0;
    std::string _name;
};

class VulkanSwapchain {
public:
    bool create(VulkanDevice &device, uint32_t width, uint32_t height, bool vsync, std::string &error);
    // 장치가 idle일 때 부른다. 창이 최소화돼 크기가 0이면 false.
    bool recreate(uint32_t width, uint32_t height, std::string &error);
    void destroy();

    VkSwapchainKHR handle() const { return _swapchain; }
    VkExtent2D extent() const { return _extent; }
    VkFormat format() const { return _format; }
    bool canReadBack() const { return _canReadBack; }
    uint32_t imageCount() const { return static_cast<uint32_t>(_images.size()); }
    GpuImage *image(uint32_t index) { return &_images[index]; }

private:
    VulkanDevice *_device = nullptr;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    VkExtent2D _extent{};
    bool _vsync = true;
    bool _canReadBack = false;  // 스왑체인 이미지를 transfer src로 쓸 수 있는지(캡처)
    std::vector<GpuImage> _images;
};

class FrameScheduler {
public:
    bool create(VulkanDevice &device, VulkanSwapchain &swapchain, uint32_t framesInFlight, std::string &error);
    void destroy();
    // 스왑체인을 다시 만든 뒤(장치 idle) 이미지 수에 맞춰 세마포어를 다시 만든다.
    bool onSwapchainRecreated(std::string &error);

    // 슬롯 fence 대기 → 이미지 획득 → 명령 버퍼 시작. 스왑체인을 다시 만들어야 하면 false + outOfDate.
    bool beginFrame(VkCommandBuffer &cmd, GpuImage *&target, bool &outOfDate);
    // 명령 버퍼 종료 → 제출 → present.
    bool endFrame(bool &outOfDate);

private:
    struct Slot {
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
    };

    VulkanDevice *_device = nullptr;
    VulkanSwapchain *_swapchain = nullptr;
    std::vector<Slot> _slots;
    std::vector<VkSemaphore> _renderFinished;  // 스왑체인 이미지마다 하나
    uint32_t _current = 0;
    uint32_t _imageIndex = 0;
};

// 이미지를 다음 사용 상태로 바꾸는 장벽. 읽기 → 읽기(같은 layout)면 장벽 없이 상태만 합친다.
void transition(VkCommandBuffer cmd, GpuImage &image, VkImageLayout layout,
                VkPipelineStageFlags2 stage, VkAccessFlags2 access);
void clearImage(VkCommandBuffer cmd, GpuImage &image, float r, float g, float b, float a);
// src 전체를 dst의 [x, y, x + width, y + height] 영역으로 복사한다.
void blitImage(VkCommandBuffer cmd, GpuImage &src, GpuImage &dst, const Rect &dstRect);

}
