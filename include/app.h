/*
 * 앱 계층: 실행 설정, 시작 단계 함수, 파일럿 실행 루프. main.cc가 include 한다.
 * - PilotConfig: 명령줄 --asset, --plugins, --width, --height, --frames N(N프레임 후 종료),
 *   --toggle-every N(N프레임마다 outline 토글, 자동 확인용), --no-outline, --no-vsync, --validation,
 *   --capture out.bmp [--capture-frame N](N번째 프레임 렌더 결과 저장).
 * - RegistryPlugins / BuildGraph(src/startup.cc): 플러그인 로드, 그래프 구성.
 *   BuildGraph는 SponzaStylizedGraph가 등록돼 있으면 그걸로, 없으면 내장 기본 조합으로 만든다.
 * - runPilot(src/app.cc): Win32 창 → GPU → 장면 → 그래프 → 루프.
 *   O 키: outline 토글 → 새 그래프를 만들어 교체, 실패하면 기존 그래프 유지. Esc: 종료.
 * - 필요: 카메라 이동(WASD), 진단 오버레이, 벤치마크 기록, 시작 보고서 파일.
 * - 범위: 엔진 내부. 구현: src/startup.cc, src/app.cc
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core.h"
#include "graph.h"
#include "pass-registry.h"
#include "plugin-manager.h"
#include "schema.h"

namespace RD {

struct PilotConfig {
    std::string assetPath;
    std::string pluginDir;
    uint32_t width = 1280;
    uint32_t height = 720;
    int frames = 0;       // 0이면 창을 닫을 때까지
    int toggleEvery = 0;  // 0이면 자동 토글 없음
    bool vsync = true;
    bool validation = false;
    bool outline = true;       // 시작할 때 outline 상태
    std::string capturePath;   // 비어 있지 않으면 captureFrame에서 렌더 결과를 BMP로 저장
    int captureFrame = 30;

    static PilotConfig fromArgs(int argc, char **argv);
};

struct PilotSettings {
    bool outline = true;
};

size_t RegistryPlugins(PluginManager &plugins, PassRegistry &registry, Schema &schemas, const std::string &dir);
std::unique_ptr<Graph> BuildGraph(const PassRegistry &registry, const PilotSettings &settings,
                                  std::vector<Diagnostic> &diagnostics);

int runPilot(const PilotConfig &config, const PassRegistry &registry);

}
