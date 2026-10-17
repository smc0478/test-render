/*
 * research.EdgeBundle 계약(mask + strength). EdgeMask(생산자)와 InkOverlay(소비자)가 서로 링크하지 않고
 * 이 헤더만 공유한다. 계약의 소유자는 EdgeMask 플러그인이다(등록도 EdgeMask가 한다).
 * - 필수 필드를 바꾸면 키 버전을 올린다.
 */
#pragma once

#include "pass-context.h"
#include "schema-field.h"

struct EdgeBundle {
    RD::Texture mask;      // R8: 결정적 edge 표시(0/1)
    RD::Texture strength;  // R16F: 연속 edge 강도(0..1)
};

DEFINE_NAMED_STRUCT_SCHEM(EdgeBundle, "research.EdgeBundle", (mask, strength))
