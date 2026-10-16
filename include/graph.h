/*
 * 실행 가능한 그래프 = GraphBuilder::compile()의 결과. 런타임은 이 단위로 실행·교체한다.
 * GPU 핸들은 넣지 않는다(GPU 자원이 붙은 형태는 runtime.h).
 * - 있는 것: Graph(위상 정렬된 PassWrap 목록, execute), PassWrap(Pass + 입력 필드별 생산자),
 *   IrEdge(생산자 위치, 출력 필드). 생성은 GraphBuilder::compile()만(friend).
 * - 필요: 출력 저장 공간과 bind, 외부 입출력, revision.
 * - 범위: 엔진 내부.
 */
#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "pass.h"

namespace RD {

class GraphBuilder;

// 입력 하나를 채우는 생산자. producer는 Graph::nodes 인덱스이고 항상 자기 노드보다 앞이다.
struct IrEdge {
    size_t producer;
    std::string outputField;
};

struct PassWrap {
    Pass *pass = nullptr;
    // 내 입력 필드 이름 → 생산자. 키가 입력 필드라 입력 하나에 생산자는 하나뿐이다.
    std::unordered_map<std::string, IrEdge> inputs;
};

class Graph {
public:
    void execute() {
        for(PassWrap &node : nodes)
            node.pass->execute();
    }

private:
    friend class GraphBuilder;
    Graph() = default;

    std::vector<PassWrap> nodes;  // 위상 정렬 순서
};

}
