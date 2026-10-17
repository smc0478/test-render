/*
 * 내장 Pass가 주고받는 core.* 데이터 타입과 그래프 입력 이름. 플러그인이 내장 Pass와 연결하려면
 * 같은 타입을 알아야 해서 공개한다. 텍스처 타입은 의미(Albedo, WorldNormal ...)마다 따로 만들어
 * 포맷이 같아도 의미가 다르면 연결되지 않게 한다.
 * - 그래프 입력(바깥에서 bind): SceneGeometry, CameraFrame, LightFrame, PresentTarget.
 * - Pass 사이 텍스처: ShadowDepth, Albedo, WorldNormal, MaterialParams, LinearDepth, Emissive, SceneHDR, LinearLDR.
 * - 필요: 키에 버전(@1), 셰이더가 생기면 포맷 규약대로 채우기.
 * - 범위: SDK 공개. 플러그인은 core.* 를 재정의할 수 없다.
 */
#pragma once

#include "pass-context.h"
#include "schema-field.h"

namespace RD {

struct SceneView;  // scene.h(엔진 내부)

// --- 그래프 입력 ---
struct SceneGeometry { const SceneView *view; };
struct CameraFrame {
    float view[16];
    float proj[16];
    float position[4];
};
struct LightFrame {
    float direction[4];
    float radiance[4];
};
struct PresentTarget { Texture tex; };

inline constexpr const char *kSceneGeometry = "scene.geometry";
inline constexpr const char *kCameraFrame = "frame.camera";
inline constexpr const char *kLightFrame = "frame.lights";
inline constexpr const char *kPresentTarget = "frame.presentTarget";

// --- Pass 사이 텍스처 ---
struct ShadowDepth { Texture tex; };     // D32, 2048x2048
struct Albedo { Texture tex; };          // RGBA16F, 화면 크기, 선형 base color
struct WorldNormal { Texture tex; };     // RGBA16F, world xyz, 배경 0
struct MaterialParams { Texture tex; };  // RGBA8: R metallic, G roughness, B occlusion
struct LinearDepth { Texture tex; };     // R32F: +view-z(m), 배경 0
struct Emissive { Texture tex; };        // RGBA16F 선형
struct SceneHDR { Texture tex; };        // RGBA16F 선형 장면색
struct LinearLDR { Texture tex; };       // RGBA8 선형 0..1(sRGB 인코딩은 스왑체인에서 한 번)

}

DEFINE_NAMED_STRUCT_SCHEM(RD::SceneGeometry, "core.SceneGeometry", (view))
DEFINE_NAMED_STRUCT_SCHEM(RD::CameraFrame, "core.CameraFrame", (view, proj, position))
DEFINE_NAMED_STRUCT_SCHEM(RD::LightFrame, "core.LightFrame", (direction, radiance))
DEFINE_NAMED_STRUCT_SCHEM(RD::PresentTarget, "core.PresentTarget", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::ShadowDepth, "core.ShadowDepth", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::Albedo, "core.Albedo", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::WorldNormal, "core.WorldNormal", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::MaterialParams, "core.MaterialParams", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::LinearDepth, "core.LinearDepth", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::Emissive, "core.Emissive", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::SceneHDR, "core.SceneHDR", (tex))
DEFINE_NAMED_STRUCT_SCHEM(RD::LinearLDR, "core.LinearLDR", (tex))
