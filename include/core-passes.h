/*
 * 내장 Pass 5개와 기본 모듈 등록. 플러그인보다 먼저 등록해야 플러그인이 이들을 참조할 수 있다.
 * - ShadowMap, GBuffer, DeferredPBR, ToneMap, Present(종단 sink).
 * - RegistryDefaultModule(): core-schemas.h의 스키마 + 위 Pass 등록(main이 호출).
 * - 범위: 엔진 내부. 구현: src/core-passes.cc
 */
