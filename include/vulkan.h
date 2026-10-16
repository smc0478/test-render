/*
 * Vulkan 백엔드. Vk* 타입은 이 파일 쌍 밖으로 내보내지 않는다.
 * - 장치(1.3, Dynamic Rendering, color attachment 5개 확인), 스왑체인(resize 시 재생성),
 *   이미지·버퍼 할당, 명령 기록·장벽 변환, 파이프라인 생성, 프레임 슬롯 2개(fence·세마포어).
 * - submit/present는 여기서만 한다.
 * - 범위: 엔진 내부. 구현: src/vulkan.cc
 */
