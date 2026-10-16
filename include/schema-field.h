/*
 * 구조체 스키마. 기존 C++ 구조체에 DEFINE_STRUCT_SCHEM(Type, (fields...)) 한 줄로 노출 필드만 지정한다.
 * - 있는 것: SchemaFieldDesc, SchemaTraits<T> 전방 선언.
 * - 필요: SchemaFieldDesc(이름, 타입, offset), 기본형 SchemaTraits 특수화, DEFINE_STRUCT_SCHEM
 *   (등록은 그래프가 포트를 모을 때), standard-layout static_assert.
 * - 범위: SDK 공개.
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
