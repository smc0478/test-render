/*
 * 그래프 실행과 교체. Graph에 GPU 자원을 붙여 매 프레임 실행하고, 새 그래프는 준비가 끝났을 때만 바꾼다.
 * - CompiledGraph: Graph + 파이프라인·이미지(프레임 슬롯별) + 사용 중인 DLL 핸들.
 * - 실행: 외부 입력(scene, camera, swapchain) 연결 → 순서대로 장벽 + execute().
 * - 교체: 프레임 경계에서만. 이전 그래프는 GPU가 다 쓴 뒤(fence) 해제한다.
 * - 범위: 엔진 내부. 구현: src/runtime.cc
 */
