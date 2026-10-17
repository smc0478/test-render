/*
 * 앱 시작 단계: 플러그인 등록과 그래프 구성. 선언은 app.h.
 * (원래 graph-build.cc — GraphBuilder 구현이 아니라 시작 절차라서 변경. GraphBuilder 구현은 graph-builder.cc)
 * - 필요: PilotApplication::initialize()로 흡수, 그래프 빌드 시간 측정.
 */
#include "app.h"
#include "core-passes.h"
#include "graph-builder.h"

#include <cstdio>

namespace RD {

static void logFromPlugin(const char *message) {
    std::printf("[plugin] %s\n", message);
}

size_t RegistryPlugins(PluginManager &plugins, PassRegistry &registry, Schema &schemas, const std::string &dir) {
    PluginHost host{kPluginAbiVersion, &registry, &schemas, logFromPlugin};
    size_t count = plugins.loadAll(dir, host);
    for(const std::string &name : plugins.loaded())
        std::printf("plugin loaded: %s\n", name.c_str());
    for(const Diagnostic &diagnostic : plugins.diagnostics())
        std::printf("%s\n", toString(diagnostic).c_str());
    return count;
}

std::unique_ptr<Graph> BuildGraph(const PassRegistry &registry, const PilotSettings &settings,
                                  std::vector<Diagnostic> &diagnostics) {
    GraphBuilder g(&registry);
    bool ok = false;
    if(registry.has("research.SponzaStylizedGraph")) {
        GraphBuildContext ctx(&registry);
        ctx.set("outline.enabled", settings.outline);
        ok = g.expand("research.SponzaStylizedGraph", ctx);
    } else {
        // 외부 GraphPass가 없을 때의 기본 조합(파일럿 성공 판정은 외부 GraphPass를 요구한다).
        ok = BuildCoreGraph(g);
    }
    std::unique_ptr<Graph> graph = ok ? g.compile() : nullptr;
    diagnostics = g.diagnostics();
    return graph;
}

}
