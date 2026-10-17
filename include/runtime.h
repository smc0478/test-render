/*
 * 그래프 실행(GPU). Pass들이 PassContext로 텍스처를 만들고 clear/blit을 기록하게 하고, 프레임을 제출·present한다.
 * - GpuRenderer: initialize(창) → prepare(graph): Pass prepare로 텍스처 생성 → renderFrame(graph):
 *   스왑체인 이미지를 frame.presentTarget으로 bind하고 record → present.
 *   그래프를 바꿀 때 releaseGraph()로 이전 그래프의 텍스처를 해제한다(GPU idle 후).
 *   requestCapture(path): 다음 프레임 결과를 BMP로 저장(확인·디버그용).
 * - 프레임 2개가 동시에 돌아도 텍스처는 하나씩이다. 같은 큐라 장벽이 프레임 사이 순서를 지킨다.
 * - 필요: 프레임 슬롯별 텍스처, 그래프 교체 시 fence 기반 지연 해제, GPU 시간 측정, 파이프라인·셰이더.
 * - 범위: 엔진 내부. 구현: src/runtime.cc
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core-schemas.h"
#include "graph.h"
#include "vulkan.h"

namespace RD {

class GpuRenderer {
public:
    GpuRenderer();
    ~GpuRenderer();

    bool initialize(void *hinstance, void *hwnd, uint32_t width, uint32_t height, bool vsync, bool validation,
                    std::string &error);
    void shutdown();

    bool prepare(Graph &graph, std::string &error);
    void releaseGraph();
    bool renderFrame(Graph &graph, std::string &error);

    void resize(uint32_t width, uint32_t height);
    void waitIdle() { _device.waitIdle(); }
    const std::string &deviceName() const { return _device.name(); }

    // 다음 프레임의 스왑체인 이미지를 GPU에서 읽어 24비트 BMP로 저장한다(화면 복사가 아니라 렌더 결과).
    void requestCapture(const std::string &path) { _capturePath = path; }

private:
    class Context;

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    FrameScheduler _frames;
    std::unique_ptr<Context> _context;
    PresentTarget _presentTarget{};
    std::string _capturePath;
    uint32_t _width = 0;
    uint32_t _height = 0;
    bool _resizePending = false;
    bool _initialized = false;
};

}
