/*
 * Pass가 받는 GPU 문맥. Pass가 Vulkan을 직접 만지지 않고 이 창구로만 자원을 만들고 명령을 기록한다.
 * - Format, Texture(불투명 GPU 이미지 핸들), Rect, PassContext(prepare: createTexture / record: clear, blit, blitTo).
 * - 장벽은 런타임이 이미지별 현재 상태를 추적해 clear/blit 직전에 자동으로 넣는다.
 * - 필요: 셰이더 단계에서 pipeline, dispatch, drawScene, 프레임 값(카메라·광원).
 * - 범위: SDK 공개. 구현: src/runtime.cc
 */
#pragma once

#include <cstdint>

#include "schema-field.h"

namespace RD {

enum class Format {
    RGBA8,
    RGBA16F,
    R8,
    R16F,
    R32F,
    D32
};

struct GpuImage;  // 런타임 안에서만 정의한다.

struct Texture {
    GpuImage *image = nullptr;
};

struct Rect {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

class PassContext {
public:
    virtual ~PassContext() = default;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    // prepare에서: 이 그래프가 소유하는 텍스처를 만든다.
    virtual Texture createTexture(Format format, uint32_t width, uint32_t height) = 0;

    // record에서
    virtual void clear(Texture target, float r, float g, float b, float a) = 0;
    // src 전체를 dst 전체(blit) 또는 dst의 일부 영역(blitTo)에 복사한다. 크기가 다르면 늘이거나 줄인다.
    virtual void blit(Texture src, Texture dst) = 0;
    virtual void blitTo(Texture src, Texture dst, Rect dstRect) = 0;
};

}

DEFINE_OPAQUE_SCHEM(RD::Texture, "rd.Texture")
