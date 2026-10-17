/*
 * 호스트 ↔ 플러그인 DLL 경계. 플러그인은 rd_plugin_entry 하나를 내보내고, 그 안에서 호스트 등록부에
 * Pass·GraphPass·스키마를 등록한다. DLL은 자기 static 등록부를 따로 가지므로 호스트 것을 넘겨받아 쓴다.
 * - PluginHost: ABI 버전, 호스트 Pass 등록부, 스키마 등록부, 로그 함수.
 * - RD_PLUGIN_ENTRY: 내보낼 함수 시그니처. ABI 버전이 다르면 플러그인이 false를 돌려준다.
 * - 지금은 같은 컴파일러·CRT로 빌드한다고 가정하고 C++ 객체를 넘긴다.
 * - 필요: 컴파일러가 달라도 되는 순수 C ABI(구조체 첫 필드 struct_size, 함수 테이블), 예외 차단.
 * - 범위: SDK 공개.
 */
#pragma once

#include <cstdint>

#include "pass-registry.h"
#include "schema.h"

namespace RD {

inline constexpr uint32_t kPluginAbiVersion = 1;

struct PluginHost {
    uint32_t abiVersion;
    PassRegistry *passes;
    Schema *schemas;
    void (*log)(const char *message);
};

using PluginEntry = bool (*)(PluginHost *host);

}

#ifdef _WIN32
#define RD_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define RD_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define RD_PLUGIN_ENTRY RD_PLUGIN_EXPORT bool rd_plugin_entry(RD::PluginHost *host)
