/*
 * 연결된 Pass들을 대표하는 그래프 객체의 기본 틀을 정의하는 파일.
 * - Graph: 시작 Pass를 가리키는 PassWrap 포인터를 보관.
 * - 그래프 생성, 실행 및 소유권 관리 API는 아직 구현하지 않았다.
 */
#pragma once

#include "pass.h"

namespace RD {

class Graph {
private:
    PassWrap *root;
};

}