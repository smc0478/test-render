/*
 * core-passes.h 구현. Pass가 커지면(셰이더가 붙으면) Pass별 파일로 나눈다.
 * - 구현할 것: 각 Pass의 셰이더와 파이프라인. 지금 record의 clear / blit은 자리표시다.
 */
#include "core-passes.h"

namespace RD {

// --- ShadowMap: TODO 셰이더 — light VP로 opaque/masked draw, depth-only ---
void ShadowMapPass::prepare(PassContext &ctx) {
    shadowDepth->tex = ctx.createTexture(Format::D32, 2048, 2048);
}

void ShadowMapPass::record(PassContext &ctx) {
    ctx.clear(shadowDepth->tex, 1.0f, 0, 0, 0);
}

// --- GBuffer: TODO 셰이더 — MRT 5장 + 내부 hardware depth ---
void GBufferPass::prepare(PassContext &ctx) {
    uint32_t w = ctx.width(), h = ctx.height();
    albedo->tex = ctx.createTexture(Format::RGBA16F, w, h);
    normal->tex = ctx.createTexture(Format::RGBA16F, w, h);
    material->tex = ctx.createTexture(Format::RGBA8, w, h);
    depth->tex = ctx.createTexture(Format::R32F, w, h);
    emissive->tex = ctx.createTexture(Format::RGBA16F, w, h);
}

void GBufferPass::record(PassContext &ctx) {
    ctx.clear(albedo->tex, 0.80f, 0.62f, 0.42f, 1);
    ctx.clear(normal->tex, 0.50f, 0.50f, 1.00f, 1);
    ctx.clear(material->tex, 0.10f, 0.70f, 1.00f, 1);
    ctx.clear(depth->tex, 0, 0, 0, 0);
    ctx.clear(emissive->tex, 0.90f, 0.25f, 0.10f, 1);
}

// --- DeferredPBR: TODO 셰이더 — 위치 복원, GGX 직접광, PCF 그림자, emissive, ambient ---
void DeferredPBRPass::prepare(PassContext &ctx) {
    hdr->tex = ctx.createTexture(Format::RGBA16F, ctx.width(), ctx.height());
}

void DeferredPBRPass::record(PassContext &ctx) {
    uint32_t w = ctx.width(), h = ctx.height();
    ctx.clear(hdr->tex, 0.25f, 0.45f, 0.85f, 1);

    // GBuffer가 실제로 넘어왔는지 보이도록 아래쪽에 썸네일로 붙인다.
    const Texture thumbnails[] = {albedo->tex, normal->tex, material->tex, emissive->tex};
    uint32_t size = h / 5, gap = size / 8;
    for(uint32_t i = 0; i < 4; ++i) {
        Rect rect{static_cast<int32_t>(gap + i * (size + gap)), static_cast<int32_t>(h - size - gap), size, size};
        if(rect.x + rect.width <= w)
            ctx.blitTo(thumbnails[i], hdr->tex, rect);
    }
}

// --- ToneMap: TODO 셰이더 — 2^exposureEV, ACES 근사. 지금은 HDR → LDR blit(0..1로 잘림) ---
void ToneMapPass::prepare(PassContext &ctx) {
    ldr->tex = ctx.createTexture(Format::RGBA8, ctx.width(), ctx.height());
}

void ToneMapPass::record(PassContext &ctx) {
    ctx.blit(hdr->tex, ldr->tex);
}

// --- Present: LinearLDR → sRGB 스왑체인(blit이 sRGB 인코딩을 한 번 한다) ---
void PresentPass::record(PassContext &ctx) {
    ctx.blit(ldr->tex, target->tex);
}

void RegistryDefaultModule(PassRegistry &registry, Schema &schemas) {
    registry.add<ShadowMapPass>("core.ShadowMap");
    registry.add<GBufferPass>("core.GBuffer");
    registry.add<DeferredPBRPass>("core.DeferredPBR");
    registry.add<ToneMapPass>("core.ToneMap");
    registry.add<PresentPass>("core.Present");

    schemas.registerSchema(SchemaTraits<SceneGeometry>::desc());
    schemas.registerSchema(SchemaTraits<CameraFrame>::desc());
    schemas.registerSchema(SchemaTraits<LightFrame>::desc());
    schemas.registerSchema(SchemaTraits<PresentTarget>::desc());
    schemas.registerSchema(SchemaTraits<ShadowDepth>::desc());
    schemas.registerSchema(SchemaTraits<Albedo>::desc());
    schemas.registerSchema(SchemaTraits<WorldNormal>::desc());
    schemas.registerSchema(SchemaTraits<MaterialParams>::desc());
    schemas.registerSchema(SchemaTraits<LinearDepth>::desc());
    schemas.registerSchema(SchemaTraits<Emissive>::desc());
    schemas.registerSchema(SchemaTraits<SceneHDR>::desc());
    schemas.registerSchema(SchemaTraits<LinearLDR>::desc());
}

bool BuildCoreGraph(GraphBuilder &g) {
    bool ok = g.addInput<SceneGeometry>(kSceneGeometry) && g.addInput<CameraFrame>(kCameraFrame) &&
              g.addInput<LightFrame>(kLightFrame) && g.addInput<PresentTarget>(kPresentTarget);
    ok = ok && g.createPass("core.ShadowMap", "shadow") && g.createPass("core.GBuffer", "gbuffer") &&
         g.createPass("core.DeferredPBR", "pbr") && g.createPass("core.ToneMap", "tonemap") &&
         g.createPass("core.Present", "present");

    ok = ok && g.connectInput(kSceneGeometry, "shadow.scene") && g.connectInput(kLightFrame, "shadow.lights");
    ok = ok && g.connectInput(kSceneGeometry, "gbuffer.scene") && g.connectInput(kCameraFrame, "gbuffer.camera");
    ok = ok && g.connect("gbuffer.albedo", "pbr.albedo") && g.connect("gbuffer.normal", "pbr.normal") &&
         g.connect("gbuffer.material", "pbr.material") && g.connect("gbuffer.depth", "pbr.depth") &&
         g.connect("gbuffer.emissive", "pbr.emissive") && g.connect("shadow.shadowDepth", "pbr.shadowDepth");
    ok = ok && g.connectInput(kCameraFrame, "pbr.camera") && g.connectInput(kLightFrame, "pbr.lights");
    ok = ok && g.connect("pbr.hdr", "tonemap.hdr") && g.connect("tonemap.ldr", "present.ldr") &&
         g.connectInput(kPresentTarget, "present.target");
    return ok;
}

}
