/*
 * 플러그인 테스트(plugin-manager.h, plugins/): 독립 빌드한 DLL 3개를 로드하고 그래프를 만든다. GPU는 쓰지 않는다.
 * - DLL이 호스트 등록부에 Pass·GraphPass·스키마를 등록한다.
 * - SponzaStylizedGraph로 outline ON이면 Pass 7개, OFF면 5개(상세.md V-05/V-06).
 * - EdgeMask·InkOverlay 없이 GraphPass만 있으면 outline ON 그래프는 실패하고 OFF는 성공.
 */
#include "app.h"
#include "core-passes.h"
#include "test.h"

#include <string>

namespace {

const std::string kPluginDir = RD_PLUGIN_DIR;

void loadAllPlugins() {
    RD::PassRegistry registry;
    RD::Schema schemas;
    RD::RegistryDefaultModule(registry, schemas);
    RD::PluginManager plugins;
    RD::PluginHost host{RD::kPluginAbiVersion, &registry, &schemas, nullptr};

    CHECK_EQ(plugins.loadAll(kPluginDir, host), size_t(3));
    CHECK(registry.has("research.EdgeMask"));
    CHECK(registry.has("research.InkOverlay"));
    CHECK(registry.has("research.SponzaStylizedGraph"));
    CHECK(schemas.find("research.EdgeBundle") != nullptr);
}

void outlineOnOffNodeCounts() {
    RD::PassRegistry registry;
    RD::Schema schemas;
    RD::RegistryDefaultModule(registry, schemas);
    RD::PluginManager plugins;
    RD::PluginHost host{RD::kPluginAbiVersion, &registry, &schemas, nullptr};
    plugins.loadAll(kPluginDir, host);

    std::vector<RD::Diagnostic> diagnostics;
    RD::PilotSettings on;
    on.outline = true;
    auto withOutline = RD::BuildGraph(registry, on, diagnostics);
    CHECK(withOutline != nullptr);

    RD::PilotSettings off;
    off.outline = false;
    auto withoutOutline = RD::BuildGraph(registry, off, diagnostics);
    CHECK(withoutOutline != nullptr);

    if(withOutline && withoutOutline) {
        CHECK_EQ(withOutline->nodes().size(), size_t(7));
        CHECK_EQ(withoutOutline->nodes().size(), size_t(5));
    }
    // 그래프(DLL 코드로 만든 Pass)를 DLL보다 먼저 없앤다.
    withOutline.reset();
    withoutOutline.reset();
}

void missingResearchPassesFailOnlyWithOutline() {
    RD::PassRegistry registry;
    RD::Schema schemas;
    RD::RegistryDefaultModule(registry, schemas);
    RD::PluginManager plugins;
    RD::PluginHost host{RD::kPluginAbiVersion, &registry, &schemas, nullptr};
    CHECK(plugins.load(kPluginDir + "/SponzaStylizedGraph.dll", host));

    std::vector<RD::Diagnostic> diagnostics;
    RD::PilotSettings on;
    on.outline = true;
    auto failed = RD::BuildGraph(registry, on, diagnostics);
    CHECK(failed == nullptr);
    CHECK(!diagnostics.empty());

    RD::PilotSettings off;
    off.outline = false;
    auto ok = RD::BuildGraph(registry, off, diagnostics);
    CHECK(ok != nullptr);
    ok.reset();
}

void wrongAbiIsRejected() {
    RD::PassRegistry registry;
    RD::Schema schemas;
    RD::PluginManager plugins;
    RD::PluginHost host{RD::kPluginAbiVersion + 1, &registry, &schemas, nullptr};
    CHECK(!plugins.load(kPluginDir + "/EdgeMask.dll", host));
    CHECK(!registry.has("research.EdgeMask"));
}

} // namespace

int main() {
    RUN(loadAllPlugins);
    RUN(outlineOnOffNodeCounts);
    RUN(missingResearchPassesFailOnlyWithOutline);
    RUN(wrongAbiIsRejected);
    return TEST_RESULT();
}
