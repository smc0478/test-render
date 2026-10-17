/*
 * 내장 Pass 5개와 기본 모듈 등록. 플러그인보다 먼저 등록해야 플러그인이 이들을 참조할 수 있다.
 * - ShadowMap, GBuffer, DeferredPBR, ToneMap, Present. 필드와 연결 규약은 상세.md §12.2 그대로다.
 * - 셰이더가 아직 없어서 clear / blit으로만 동작한다: GBuffer는 출력마다 다른 색으로 지우고,
 *   DeferredPBR은 하늘색 HDR에 GBuffer 결과를 작은 썸네일로 붙이고, ToneMap·Present는 blit한다.
 * - RegistryDefaultModule: core.* 스키마 + 위 Pass 등록(main이 호출).
 * - BuildCoreGraph: 외부 GraphPass가 없을 때 쓰는 기본 조합(그래프 입력 4개 + Pass 5개).
 * - 필요: 각 Pass의 Slang 셰이더(depth-only, MRT, GGX, ACES, fullscreen).
 * - 범위: 엔진 내부. 구현: src/core-passes.cc
 */
#pragma once

#include "core-schemas.h"
#include "graph-builder.h"
#include "pass-field.h"
#include "pass-registry.h"
#include "schema.h"

namespace RD {

class ShadowMapPass : public Pass {
public:
    void prepare(PassContext &ctx) override;
    void record(PassContext &ctx) override;
private:
    DECL_FIELD_IN(SceneGeometry, scene);
    DECL_FIELD_IN(LightFrame, lights);
    DECL_FIELD_OUT(ShadowDepth, shadowDepth);
};

class GBufferPass : public Pass {
public:
    void prepare(PassContext &ctx) override;
    void record(PassContext &ctx) override;
private:
    DECL_FIELD_IN(SceneGeometry, scene);
    DECL_FIELD_IN(CameraFrame, camera);
    DECL_FIELD_OUT(Albedo, albedo);
    DECL_FIELD_OUT(WorldNormal, normal);
    DECL_FIELD_OUT(MaterialParams, material);
    DECL_FIELD_OUT(LinearDepth, depth);
    DECL_FIELD_OUT(Emissive, emissive);
};

class DeferredPBRPass : public Pass {
public:
    void prepare(PassContext &ctx) override;
    void record(PassContext &ctx) override;
private:
    DECL_FIELD_IN(Albedo, albedo);
    DECL_FIELD_IN(WorldNormal, normal);
    DECL_FIELD_IN(MaterialParams, material);
    DECL_FIELD_IN(LinearDepth, depth);
    DECL_FIELD_IN(Emissive, emissive);
    DECL_FIELD_IN(ShadowDepth, shadowDepth);
    DECL_FIELD_IN(CameraFrame, camera);
    DECL_FIELD_IN(LightFrame, lights);
    DECL_FIELD_OUT(SceneHDR, hdr);
};

class ToneMapPass : public Pass {
public:
    void prepare(PassContext &ctx) override;
    void record(PassContext &ctx) override;
private:
    DECL_FIELD_IN(SceneHDR, hdr);
    DECL_FIELD_OUT(LinearLDR, ldr);
};

// 종단 sink: 데이터 출력 없이 획득한 스왑체인 이미지에 쓴다.
class PresentPass : public Pass {
public:
    void record(PassContext &ctx) override;
private:
    DECL_FIELD_IN(LinearLDR, ldr);
    DECL_FIELD_IN(PresentTarget, target);
};

void RegistryDefaultModule(PassRegistry &registry, Schema &schemas);
bool BuildCoreGraph(GraphBuilder &g);

}
