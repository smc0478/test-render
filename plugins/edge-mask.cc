/*
 * EdgeMask.dll — GBuffer의 normal / depth 차이로 윤곽선을 검출하는 Pass(research.EdgeMask).
 * SDK 헤더만으로 독립 빌드하는 Pass의 예시이고, research.EdgeBundle 스키마도 이 DLL이 등록한다.
 * - 입력 normal(core.WorldNormal), depth(core.LinearDepth) → 출력 edge(research.EdgeBundle).
 * - 필요: edge_mask.slang(4/8-이웃 depth·normal 차이). 지금은 mask를 1로 지우는 자리표시.
 */
#include "core-schemas.h"
#include "edge-bundle.h"
#include "pass-field.h"
#include "plugin-api.h"

namespace {

class EdgeMaskPass : public RD::Pass {
public:
    void prepare(RD::PassContext &ctx) override {
        edge->mask = ctx.createTexture(RD::Format::R8, ctx.width(), ctx.height());
        edge->strength = ctx.createTexture(RD::Format::R16F, ctx.width(), ctx.height());
    }

    void record(RD::PassContext &ctx) override {
        ctx.clear(edge->mask, 1.0f, 0, 0, 1);
        ctx.clear(edge->strength, 0.5f, 0, 0, 1);
    }

private:
    DECL_FIELD_IN(RD::WorldNormal, normal);
    DECL_FIELD_IN(RD::LinearDepth, depth);
    DECL_FIELD_OUT(EdgeBundle, edge);
};

}

RD_PLUGIN_ENTRY {
    if(host == nullptr || host->abiVersion != RD::kPluginAbiVersion)
        return false;
    if(!host->schemas->registerSchema(RD::SchemaTraits<EdgeBundle>::desc()))
        return false;
    return host->passes->add<EdgeMaskPass>("research.EdgeMask");
}
