/*
 * Sponza 장면 로드와 카메라·광원. 그래프 외부 입력(scene.*, frame.camera, frame.lights)을 공급한다.
 * - 로드: GLB 확인(LFS 포인터면 실패) → glTF 파싱 → 검증·통계 → GPU 업로드.
 * - SceneView: Pass가 읽는 읽기 전용 장면(draw 목록, 머티리얼).
 * - Camera / Light: CPU 상태와 프레임마다 GPU로 보내는 데이터.
 * - 범위: 엔진 내부. 구현: src/scene.cc
 */
