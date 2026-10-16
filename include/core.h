/*
 * 모든 파일이 쓰는 기초 타입. 표준 라이브러리만 써서 GPU·DLL 없이 테스트할 수 있게 한다.
 * - StrongId<Tag> / NodeId, EdgeId, ResourceId: 서로 섞이지 않는 ID(포인터·GPU 핸들 아님).
 * - QualifiedName("core.GBuffer"), Version: DLL 간 이름 충돌 방지, 호환 판단.
 * - Error, Result<T>, RD_TRY: 예외 대신 실패를 값으로 반환(실패한 그래프 후보만 버리기 위해).
 * - PropertyMap: Pass 파라미터·설정 값(outline.enabled 등).
 * - 범위: SDK 공개. 구현: src/core.cc
 */
