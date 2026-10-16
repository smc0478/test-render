/*
 * Pass 종류 등록부. addPass("core.GBuffer", "gbuffer")처럼 타입 이름으로 Pass를 만들기 위해 둔다.
 * - PassTypeKey(이름 + 버전), PassFactory, PassRegistry(등록·조회·생성), REGISTER_PASS 매크로.
 * - GraphPass: 실행 대신 expand(GraphBuilder&, 설정)로 하위 그래프를 만드는 Pass
 *   (예: SponzaStylizedGraph). 재구성 때만 호출하고 GPU 기록은 하지 않는다.
 * - 범위: SDK 공개. 구현: src/pass-registry.cc
 */
