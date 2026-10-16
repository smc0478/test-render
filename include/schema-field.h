/*
 * 구조체에서 스키마에 노출할 필드와 타입별 스키마 선언을 준비하는 파일.
 * - SchemaFieldDesc 전방 선언: 필드 이름, 타입, 실제 offset을 정의할 자리.
 * - SchemaTraits<T> 전방 선언: 데이터 타입과 스키마 정의를 연결할 자리.
 * - DEFINE_STRUCT_SCHEM(Type, (fields...)) 매크로는 이 파일에 구현할 예정이다.
 * - 현재는 전방 선언만 있으며 필드 메타데이터 생성과 매크로는 미구현이다.
 */
#pragma once

namespace RD {

// 노출 필드의 이름, 타입, 실제 데이터 내 offset을 정의할 자리.
struct SchemaFieldDesc;

// DEFINE_STRUCT_SCHEM(Type, (fields...))가 제공할 타입별 스키마 정보.
// 매크로와 특수화 구현은 이후 추가한다.
template <typename T>
struct SchemaTraits;

}
