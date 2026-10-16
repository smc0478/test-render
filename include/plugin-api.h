/*
 * 호스트 ↔ 플러그인 DLL 경계. 컴파일러·STL이 달라도 되게 경계에서는 C 구조체·함수 포인터만 쓴다.
 * - C ABI: RD_HostApi(등록 콜백), RD_PassCallbacks, RD_PluginExports, rd_plugin_entry 심볼.
 * - RD_PLUGIN(...) 매크로: C++ Pass 등록을 C ABI로 이어 주는 편의 계층.
 * - 규칙: 구조체 첫 필드는 struct_size, 예외를 경계 밖으로 던지지 않는다.
 * - 범위: SDK 공개.
 */
