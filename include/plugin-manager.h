/*
 * plugins 폴더의 DLL을 찾아 로드·등록한다. 앱을 다시 빌드하지 않고 Pass·스키마를 추가하기 위해 둔다.
 * - 로드 → ABI 검사 → 등록(전부 성공할 때만 반영, 실패하면 되돌림) → 쓰는 동안 unload 금지.
 * - 실패한 DLL만 거부한다. hot-reload는 범위 밖.
 * - 범위: 엔진 내부. 구현: src/plugin-manager.cc
 */
