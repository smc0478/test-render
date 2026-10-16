/*
 * Pass가 받는 실행 문맥. Pass가 GPU를 직접 만지지 않고 자기가 선언한 필드의 자원만 쓰게 하는 창구다.
 * - PassContext(execute마다): input/output 자원 조회, dispatch, drawScene, 프레임 값(카메라·광원·설정).
 * - 준비 단계(그래프 준비 때 한 번): 파이프라인·셰이더 준비.
 * - GPU 타입은 불투명 핸들로만 노출한다.
 * - 범위: SDK 공개. 구현: src/runtime.cc
 */
