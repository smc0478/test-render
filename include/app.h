/*
 * 앱 계층: 시작 함수, 설정, 창·입력, 진단. main.cc가 include 한다.
 * - RegistryPlugins(), BuildGraph(): 시작 단계(src/startup.cc).
 * - 설정: 그래프가 바뀌는 설정(outline.enabled → 재구성)과 상수만 바뀌는 설정(강도 → uniform)을 구분.
 * - 창·입력(WASD, O 토글), 오류 출력, 프레임 시간 기록.
 * - 범위: 엔진 내부. 구현: src/startup.cc, src/app.cc
 */
