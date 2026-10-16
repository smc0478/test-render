/*
 * 그래프 안의 Pass와 다음 Pass로 이어지는 연결 정보를 정의하는 파일.
 * - IrEdge: 다음 Pass 포인터와 입력 필드 이름 → 출력 필드 이름의 대응표.
 * - PassWrap: 현재 GraphIr 포인터와 다음 Pass로 향하는 연결 목록.
 * - 현재는 저장 구조의 초안이며 연결 관리와 실행 처리는 미구현이다.
 */
#pragma once

#include "graph-ir.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace RD {

struct IrEdge {
    PassWrap *nextPass;

    // key   = nextIr의 Input 필드
    // value = currentIr의 Output 필드
    std::unordered_map<std::string, std::string> inoutMap;
};

class PassWrap {
private:
    GraphIr *currentIr;
    std::vector<IrEdge> edges;
};

}