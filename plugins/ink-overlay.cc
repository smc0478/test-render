/*
 * InkOverlay.dll — EdgeBundle을 받아 HDR 이미지에 잉크 선을 합성하는 Pass(research.InkOverlay).
 * 다른 DLL의 계약(EdgeBundle)을 헤더 공유만으로 쓰는 예시 — EdgeMask.dll에 링크하지 않는다.
 * - 입력 hdrIn(core.SceneHDR), edge(research.EdgeBundle) → 출력 hdr(core.SceneHDR, 새 이미지).
 * - 필요: ink_overlay.slang(alpha = saturate(mask * strength * inkStrength)로 혼합).
 *   지금은 hdrIn을 복사하고 edge.mask를 화면 테두리에 붙여 outline ON/OFF가 보이게만 한다.
 */
#include <algorithm>

#include "core-schemas.h"
#include "edge-bundle.h"
#include "pass-field.h"
#include "plugin-api.h"

namespace {

class InkOverlayPass : public RD::Pass {
public:
    void prepare(RD::PassContext &ctx) override {
        hdr->tex = ctx.createTexture(RD::Format::RGBA16F, ctx.width(), ctx.height());
    }

    void record(RD::PassContext &ctx) override {
        ctx.blit(hdrIn->tex, hdr->tex);

        uint32_t w = ctx.width(), h = ctx.height();
        uint32_t t = std::max(4u, h / 60);
        ctx.blitTo(edge->mask, hdr->tex, RD::Rect{0, 0, w, t});
        ctx.blitTo(edge->mask, hdr->tex, RD::Rect{0, static_cast<int32_t>(h - t), w, t});
        ctx.blitTo(edge->mask, hdr->tex, RD::Rect{0, 0, t, h});
        ctx.blitTo(edge->mask, hdr->tex, RD::Rect{static_cast<int32_t>(w - t), 0, t, h});
    }

private:
    DECL_FIELD_IN(RD::SceneHDR, hdrIn);
    DECL_FIELD_IN(EdgeBundle, edge);
    DECL_FIELD_OUT(RD::SceneHDR, hdr);
};

}

RD_PLUGIN_ENTRY {
    if(host == nullptr || host->abiVersion != RD::kPluginAbiVersion)
        return false;
    return host->passes->add<InkOverlayPass>("research.InkOverlay");
}
