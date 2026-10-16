/*
 * Slang 셰이더 컴파일과 바인딩 검사. Slang 의존을 이 파일 쌍에 가둔다.
 * - 컴파일: Slang → SPIR-V + 반사 정보(바인딩 이름·set·binding). 같은 입력이면 캐시 재사용.
 * - 검사: Pass 필드 선언과 셰이더 바인딩이 맞는지 GPU 실행 전에 확인한다.
 * - 범위: 엔진 내부. 구현: src/shader.cc
 */
