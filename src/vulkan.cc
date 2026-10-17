/*
 * vulkan.h 구현.
 * - 구현할 것: 이미지 뷰·파이프라인(셰이더 단계), 검증 레이어 메시지 출력, 메모리 할당기.
 */
#include "vulkan.h"

#include <algorithm>
#include <cstring>

namespace RD {

VkFormat toVkFormat(Format format) {
    switch(format) {
    case Format::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::R8: return VK_FORMAT_R8_UNORM;
    case Format::R16F: return VK_FORMAT_R16_SFLOAT;
    case Format::R32F: return VK_FORMAT_R32_SFLOAT;
    case Format::D32: return VK_FORMAT_D32_SFLOAT;
    }
    return VK_FORMAT_UNDEFINED;
}

// --- VulkanDevice ---

static bool hasLayer(const char *name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for(const VkLayerProperties &layer : layers)
        if(std::strcmp(layer.layerName, name) == 0)
            return true;
    return false;
}

static bool hasDeviceExtension(VkPhysicalDevice physical, const char *name) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, extensions.data());
    for(const VkExtensionProperties &extension : extensions)
        if(std::strcmp(extension.extensionName, name) == 0)
            return true;
    return false;
}

bool VulkanDevice::initialize(void *hinstance, void *hwnd, bool validation, std::string &error) {
    if(volkInitialize() != VK_SUCCESS) {
        error = "vulkan-1.dll not found";
        return false;
    }
    if(volkGetInstanceVersion() < VK_API_VERSION_1_3) {
        error = "Vulkan 1.3 loader required";
        return false;
    }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "RendererPilot";
    app.apiVersion = VK_API_VERSION_1_3;

    const char *extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    const char *validationLayer = "VK_LAYER_KHRONOS_validation";
    bool useValidation = validation && hasLayer(validationLayer);

    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = extensions;
    ici.enabledLayerCount = useValidation ? 1 : 0;
    ici.ppEnabledLayerNames = &validationLayer;
    if(vkCreateInstance(&ici, nullptr, &_instance) != VK_SUCCESS) {
        error = "vkCreateInstance failed";
        return false;
    }
    volkLoadInstance(_instance);

    VkWin32SurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    sci.hinstance = static_cast<HINSTANCE>(hinstance);
    sci.hwnd = static_cast<HWND>(hwnd);
    if(vkCreateWin32SurfaceKHR(_instance, &sci, nullptr, &_surface) != VK_SUCCESS) {
        error = "vkCreateWin32SurfaceKHR failed";
        return false;
    }

    // graphics + present 큐가 있고 1.3·swapchain을 지원하는 장치 중 discrete 우선.
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(_instance, &count, nullptr);
    std::vector<VkPhysicalDevice> physicals(count);
    vkEnumeratePhysicalDevices(_instance, &count, physicals.data());
    int bestScore = -1;
    for(VkPhysicalDevice physical : physicals) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical, &props);
        if(props.apiVersion < VK_API_VERSION_1_3 || !hasDeviceExtension(physical, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            continue;

        VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        f2.pNext = &f13;
        vkGetPhysicalDeviceFeatures2(physical, &f2);
        if(!f13.synchronization2)
            continue;

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &familyCount, families.data());
        for(uint32_t i = 0; i < familyCount; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, _surface, &present);
            if(!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !present)
                continue;
            int score = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
            if(score > bestScore) {
                bestScore = score;
                _physical = physical;
                _queueFamily = i;
                _name = props.deviceName;
            }
            break;
        }
    }
    if(_physical == VK_NULL_HANDLE) {
        error = "no Vulkan 1.3 device with graphics + present + synchronization2";
        return false;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = _queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    f2.pNext = &f13;

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.pNext = &f2;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = deviceExtensions;
    if(vkCreateDevice(_physical, &dci, nullptr, &_device) != VK_SUCCESS) {
        error = "vkCreateDevice failed";
        return false;
    }
    volkLoadDevice(_device);
    vkGetDeviceQueue(_device, _queueFamily, 0, &_queue);
    return true;
}

void VulkanDevice::shutdown() {
    if(_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }
    if(_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
    if(_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}

void VulkanDevice::waitIdle() {
    if(_device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(_device);
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(_physical, &props);
    for(uint32_t i = 0; i < props.memoryTypeCount; ++i)
        if((typeBits & (1u << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return UINT32_MAX;
}

GpuImage *VulkanDevice::createImage(Format format, uint32_t width, uint32_t height) {
    bool depth = format == Format::D32;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = toVkFormat(format);
    ci.extent = {width, height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
               (depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    GpuImage *image = new GpuImage();
    image->format = ci.format;
    image->aspect = depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image->extent = {width, height};
    if(vkCreateImage(_device, &ci, nullptr, &image->image) != VK_SUCCESS) {
        delete image;
        return nullptr;
    }

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(_device, image->image, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(ai.memoryTypeIndex == UINT32_MAX || vkAllocateMemory(_device, &ai, nullptr, &image->memory) != VK_SUCCESS) {
        destroyImage(image);
        return nullptr;
    }
    vkBindImageMemory(_device, image->image, image->memory, 0);
    return image;
}

GpuBuffer VulkanDevice::createReadbackBuffer(VkDeviceSize size) {
    GpuBuffer buffer;
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size = size;
    ci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if(vkCreateBuffer(_device, &ci, nullptr, &buffer.buffer) != VK_SUCCESS)
        return GpuBuffer{};

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(_device, buffer.buffer, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex =
        findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if(ai.memoryTypeIndex == UINT32_MAX || vkAllocateMemory(_device, &ai, nullptr, &buffer.memory) != VK_SUCCESS) {
        destroyBuffer(buffer);
        return GpuBuffer{};
    }
    vkBindBufferMemory(_device, buffer.buffer, buffer.memory, 0);
    buffer.size = size;
    return buffer;
}

void VulkanDevice::destroyBuffer(GpuBuffer &buffer) {
    if(buffer.buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(_device, buffer.buffer, nullptr);
    if(buffer.memory != VK_NULL_HANDLE)
        vkFreeMemory(_device, buffer.memory, nullptr);
    buffer = GpuBuffer{};
}

void VulkanDevice::destroyImage(GpuImage *image) {
    if(image == nullptr)
        return;
    if(image->owned) {
        vkDestroyImage(_device, image->image, nullptr);
        vkFreeMemory(_device, image->memory, nullptr);
    }
    delete image;
}

// --- VulkanSwapchain ---

bool VulkanSwapchain::create(VulkanDevice &device, uint32_t width, uint32_t height, bool vsync, std::string &error) {
    _device = &device;
    _vsync = vsync;
    return recreate(width, height, error);
}

bool VulkanSwapchain::recreate(uint32_t width, uint32_t height, std::string &error) {
    VkPhysicalDevice physical = _device->physicalDevice();
    VkSurfaceKHR surface = _device->surface();

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);
    VkExtent2D extent = caps.currentExtent;
    if(extent.width == UINT32_MAX) {
        extent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if(extent.width == 0 || extent.height == 0) {
        error = "window minimized";
        return false;
    }
    if(!(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        error = "swapchain does not support transfer dst";
        return false;
    }

    // sRGB 스왑체인이면 선형 LDR을 blit할 때 sRGB 인코딩이 한 번만 일어난다.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, formats.data());
    VkSurfaceFormatKHR chosen = formats[0];
    for(const VkSurfaceFormatKHR &f : formats)
        if(f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            chosen = f;

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &modeCount, modes.data());
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if(!_vsync)
        for(VkPresentModeKHR mode : modes)
            if(mode == VK_PRESENT_MODE_MAILBOX_KHR || mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                presentMode = mode;

    uint32_t imageCount = caps.minImageCount + 1;
    if(caps.maxImageCount > 0)
        imageCount = std::min(imageCount, caps.maxImageCount);

    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = chosen.format;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    _canReadBack = (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
    ci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    (_canReadBack ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = presentMode;
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = _swapchain;

    VkSwapchainKHR created = VK_NULL_HANDLE;
    if(vkCreateSwapchainKHR(_device->device(), &ci, nullptr, &created) != VK_SUCCESS) {
        error = "vkCreateSwapchainKHR failed";
        return false;
    }
    if(_swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
    _swapchain = created;
    _format = chosen.format;
    _extent = extent;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(_device->device(), _swapchain, &count, nullptr);
    std::vector<VkImage> images(count);
    vkGetSwapchainImagesKHR(_device->device(), _swapchain, &count, images.data());
    _images.assign(count, GpuImage{});
    for(uint32_t i = 0; i < count; ++i) {
        _images[i].image = images[i];
        _images[i].format = _format;
        _images[i].extent = _extent;
        _images[i].owned = false;
    }
    return true;
}

void VulkanSwapchain::destroy() {
    if(_swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
    _swapchain = VK_NULL_HANDLE;
    _images.clear();
}

// --- FrameScheduler ---

static void destroySemaphores(VkDevice device, std::vector<VkSemaphore> &semaphores) {
    for(VkSemaphore s : semaphores)
        vkDestroySemaphore(device, s, nullptr);
    semaphores.clear();
}

bool FrameScheduler::create(VulkanDevice &device, VulkanSwapchain &swapchain, uint32_t framesInFlight,
                            std::string &error) {
    _device = &device;
    _swapchain = &swapchain;
    VkDevice dev = device.device();
    _slots.resize(framesInFlight);
    for(Slot &slot : _slots) {
        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pci.queueFamilyIndex = device.queueFamily();
        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        if(vkCreateCommandPool(dev, &pci, nullptr, &slot.pool) != VK_SUCCESS) {
            error = "command pool creation failed";
            return false;
        }
        cai.commandPool = slot.pool;
        if(vkAllocateCommandBuffers(dev, &cai, &slot.cmd) != VK_SUCCESS ||
           vkCreateFence(dev, &fci, nullptr, &slot.fence) != VK_SUCCESS ||
           vkCreateSemaphore(dev, &sci, nullptr, &slot.imageAvailable) != VK_SUCCESS) {
            error = "frame slot creation failed";
            return false;
        }
    }
    return onSwapchainRecreated(error);
}

bool FrameScheduler::onSwapchainRecreated(std::string &error) {
    VkDevice dev = _device->device();
    destroySemaphores(dev, _renderFinished);
    _renderFinished.resize(_swapchain->imageCount(), VK_NULL_HANDLE);
    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for(VkSemaphore &s : _renderFinished)
        if(vkCreateSemaphore(dev, &sci, nullptr, &s) != VK_SUCCESS) {
            error = "semaphore creation failed";
            return false;
        }
    return true;
}

void FrameScheduler::destroy() {
    if(_device == nullptr)
        return;
    VkDevice dev = _device->device();
    for(Slot &slot : _slots) {
        vkDestroySemaphore(dev, slot.imageAvailable, nullptr);
        vkDestroyFence(dev, slot.fence, nullptr);
        vkDestroyCommandPool(dev, slot.pool, nullptr);
    }
    _slots.clear();
    destroySemaphores(dev, _renderFinished);
}

bool FrameScheduler::beginFrame(VkCommandBuffer &cmd, GpuImage *&target, bool &outOfDate) {
    VkDevice dev = _device->device();
    Slot &slot = _slots[_current];
    vkWaitForFences(dev, 1, &slot.fence, VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(dev, _swapchain->handle(), UINT64_MAX, slot.imageAvailable,
                                            VK_NULL_HANDLE, &_imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        outOfDate = true;
        return false;
    }
    if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        return false;

    vkResetFences(dev, 1, &slot.fence);
    vkResetCommandBuffer(slot.cmd, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(slot.cmd, &bi);

    cmd = slot.cmd;
    target = _swapchain->image(_imageIndex);
    // 획득 세마포어를 ALL_COMMANDS에서 기다리므로 그 이후에는 내용을 버리고(UNDEFINED) 새로 쓴다.
    target->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    target->stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    target->access = VK_ACCESS_2_NONE;
    return true;
}

bool FrameScheduler::endFrame(bool &outOfDate) {
    Slot &slot = _slots[_current];
    vkEndCommandBuffer(slot.cmd);

    VkSemaphoreSubmitInfo wait{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait.semaphore = slot.imageAvailable;
    wait.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSemaphoreSubmitInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = _renderFinished[_imageIndex];
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkCommandBufferSubmitInfo cbi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cbi.commandBuffer = slot.cmd;

    VkSubmitInfo2 si{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    si.waitSemaphoreInfoCount = 1;
    si.pWaitSemaphoreInfos = &wait;
    si.commandBufferInfoCount = 1;
    si.pCommandBufferInfos = &cbi;
    si.signalSemaphoreInfoCount = 1;
    si.pSignalSemaphoreInfos = &signal;
    if(vkQueueSubmit2(_device->queue(), 1, &si, slot.fence) != VK_SUCCESS)
        return false;

    VkSwapchainKHR swapchain = _swapchain->handle();
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &signal.semaphore;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;
    pi.pImageIndices = &_imageIndex;
    VkResult result = vkQueuePresentKHR(_device->queue(), &pi);

    _current = (_current + 1) % static_cast<uint32_t>(_slots.size());
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        outOfDate = true;
    return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR;
}

// --- 장벽과 transfer 명령 ---

void transition(VkCommandBuffer cmd, GpuImage &image, VkImageLayout layout,
                VkPipelineStageFlags2 stage, VkAccessFlags2 access) {
    const VkAccessFlags2 writes = VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_SHADER_WRITE_BIT |
                                  VK_ACCESS_2_MEMORY_WRITE_BIT;
    bool prevWrite = (image.access & writes) != 0;
    bool nextWrite = (access & writes) != 0;
    if(image.layout == layout && !prevWrite && !nextWrite) {
        image.stage |= stage;
        image.access |= access;
        return;
    }

    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = image.stage;
    barrier.srcAccessMask = image.access;
    barrier.dstStageMask = stage;
    barrier.dstAccessMask = access;
    barrier.oldLayout = image.layout;
    barrier.newLayout = layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    barrier.subresourceRange = {image.aspect, 0, 1, 0, 1};

    VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dep);

    image.layout = layout;
    image.stage = stage;
    image.access = access;
}

void clearImage(VkCommandBuffer cmd, GpuImage &image, float r, float g, float b, float a) {
    transition(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_CLEAR_BIT,
               VK_ACCESS_2_TRANSFER_WRITE_BIT);
    VkImageSubresourceRange range{image.aspect, 0, 1, 0, 1};
    if(image.aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        VkClearDepthStencilValue value{r, 0};
        vkCmdClearDepthStencilImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
    } else {
        VkClearColorValue value{};
        value.float32[0] = r;
        value.float32[1] = g;
        value.float32[2] = b;
        value.float32[3] = a;
        vkCmdClearColorImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);
    }
}

void blitImage(VkCommandBuffer cmd, GpuImage &src, GpuImage &dst, const Rect &dstRect) {
    if(&src == &dst || (src.aspect & VK_IMAGE_ASPECT_DEPTH_BIT) || (dst.aspect & VK_IMAGE_ASPECT_DEPTH_BIT))
        return;
    transition(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_BLIT_BIT,
               VK_ACCESS_2_TRANSFER_READ_BIT);
    transition(cmd, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_BLIT_BIT,
               VK_ACCESS_2_TRANSFER_WRITE_BIT);

    VkImageBlit region{};
    region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.srcOffsets[1] = {static_cast<int32_t>(src.extent.width), static_cast<int32_t>(src.extent.height), 1};
    region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.dstOffsets[0] = {dstRect.x, dstRect.y, 0};
    region.dstOffsets[1] = {dstRect.x + static_cast<int32_t>(dstRect.width),
                            dstRect.y + static_cast<int32_t>(dstRect.height), 1};
    vkCmdBlitImage(cmd, src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
}

}
