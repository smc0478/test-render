/*
 * 프로그램 진입점. 시작 순서(기본 모듈 → 플러그인 → 실행)를 한눈에 보이게 두고, 실패하면 렌더링 전에 끝낸다.
 * 플러그인 DLL은 그 코드로 만든 Pass(그래프)가 모두 사라진 뒤에 내린다.
 */
#include "app.h"
#include "core-passes.h"

int main(int argc, char **argv) {
    RD::PilotConfig config = RD::PilotConfig::fromArgs(argc, argv);

    RD::PassRegistry registry;
    RD::Schema schemas;
    RD::RegistryDefaultModule(registry, schemas);

    RD::PluginManager plugins;
    RD::RegistryPlugins(plugins, registry, schemas, config.pluginDir);

    int result = RD::runPilot(config, registry);
    plugins.unloadAll();
    return result;
}
