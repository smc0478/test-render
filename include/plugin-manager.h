/*
 * plugins 폴더의 DLL을 찾아 로드·등록한다. 앱을 다시 빌드하지 않고 Pass·스키마를 추가하기 위해 둔다.
 * - 로드 → rd_plugin_entry 호출(ABI 확인·등록) → 앱이 끝날 때까지 unload하지 않는다.
 *   DLL 코드로 만든 Pass가 살아 있는 동안 DLL이 내려가면 안 되므로, 그래프를 먼저 없앤 뒤 unloadAll.
 * - 실패한 DLL만 거부하고 나머지는 계속 로드한다. 파일 이름 순서로 로드한다.
 * - 필요: 등록 트랜잭션(실패 시 되돌림), 의존 순서 계산, ModuleLease.
 * - 범위: 엔진 내부. 구현: src/plugin-manager.cc
 */
#pragma once

#include <string>
#include <vector>

#include "core.h"
#include "plugin-api.h"

namespace RD {

class PluginManager {
public:
    ~PluginManager();

    // dir 안의 *.dll을 모두 로드한다. 등록에 성공한 개수를 돌려준다.
    size_t loadAll(const std::string &dir, PluginHost &host);
    bool load(const std::string &path, PluginHost &host);
    void unloadAll();

    const std::vector<std::string> &loaded() const { return _names; }
    const std::vector<Diagnostic> &diagnostics() const { return _diagnostics; }

private:
    std::vector<void *> _modules;
    std::vector<std::string> _names;
    std::vector<Diagnostic> _diagnostics;
};

}
