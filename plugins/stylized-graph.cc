/*
 * SponzaStylizedGraph.dll — 내장 Pass와 다른 DLL의 Pass를 골라 전체 렌더 그래프를 만드는 GraphPass
 * (research.SponzaStylizedGraph). outline.enabled면 EdgeMask·InkOverlay를 끼워 7개, 아니면 5개 Pass.
 * GPU 명령은 기록하지 않는다. 필요한 Pass 타입이 없으면 expand가 실패하고 앱은 기존 그래프를 유지한다.
 */
#include "core-schemas.h"
#include "graph-builder.h"
#include "plugin-api.h"

namespace {

class SponzaStylizedGraphPass : public RD::GraphPass {
public:
    bool expand(RD::GraphBuilder &g, const RD::GraphBuildContext &ctx) override {
        bool ok = g.addInput<RD::SceneGeometry>(RD::kSceneGeometry) &&
                  g.addInput<RD::CameraFrame>(RD::kCameraFrame) &&
                  g.addInput<RD::LightFrame>(RD::kLightFrame) &&
                  g.addInput<RD::PresentTarget>(RD::kPresentTarget);

        ok = ok && g.createPass("core.ShadowMap", "shadow") && g.createPass("core.GBuffer", "gbuffer") &&
             g.createPass("core.DeferredPBR", "pbr") && g.createPass("core.ToneMap", "tonemap") &&
             g.createPass("core.Present", "present");

        ok = ok && g.connectInput(RD::kSceneGeometry, "shadow.scene") &&
             g.connectInput(RD::kLightFrame, "shadow.lights") &&
             g.connectInput(RD::kSceneGeometry, "gbuffer.scene") &&
             g.connectInput(RD::kCameraFrame, "gbuffer.camera");
        ok = ok && g.connect("gbuffer.albedo", "pbr.albedo") && g.connect("gbuffer.normal", "pbr.normal") &&
             g.connect("gbuffer.material", "pbr.material") && g.connect("gbuffer.depth", "pbr.depth") &&
             g.connect("gbuffer.emissive", "pbr.emissive") && g.connect("shadow.shadowDepth", "pbr.shadowDepth") &&
             g.connectInput(RD::kCameraFrame, "pbr.camera") && g.connectInput(RD::kLightFrame, "pbr.lights");

        std::string hdrSource = "pbr.hdr";
        if(ctx.getBool("outline.enabled", true)) {
            ok = ok && g.createPass("research.EdgeMask", "edge") && g.createPass("research.InkOverlay", "ink");
            ok = ok && g.connect("gbuffer.normal", "edge.normal") && g.connect("gbuffer.depth", "edge.depth") &&
                 g.connect("edge.edge", "ink.edge") && g.connect("pbr.hdr", "ink.hdrIn");
            hdrSource = "ink.hdr";
        }

        ok = ok && g.connect(hdrSource, "tonemap.hdr") && g.connect("tonemap.ldr", "present.ldr") &&
             g.connectInput(RD::kPresentTarget, "present.target");
        return ok;
    }
};

}

RD_PLUGIN_ENTRY {
    if(host == nullptr || host->abiVersion != RD::kPluginAbiVersion)
        return false;
    return host->passes->addGraph<SponzaStylizedGraphPass>("research.SponzaStylizedGraph");
}
