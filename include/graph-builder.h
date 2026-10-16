/*
 * 그래프 조립 API. 호스트와 GraphPass 플러그인이 같은 API로 Pass를 추가하고 필드를 연결한다.
 * - 있는 것: Endpoint(Pass 인덱스 + 필드), Edge, GraphBuilder(Pass를 추가 순서 인덱스로 보관,
 *   addPass·connect·compile).
 * - 필요: 그래프 입력/출력(Endpoint 종류), 실패 이유를 담는 Result 반환,
 *   addPass는 pass-registry로 생성(6번), compile의 bind.
 * - 범위: SDK 공개. 구현: src/graph-builder.cc
 */
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "pass.h"
#include "schema.h"
#include "graph.h"

namespace RD {

// 연결 한쪽 끝. pass는 GraphBuilder::passList 인덱스.
// TODO: 그래프 입력/출력 종류는 외부 입력이 생길 때, 하위 필드는 스키마(5번 이후)
struct Endpoint {
    size_t pass;
    std::string field;
};

struct Edge {
    Endpoint src;
    Endpoint dst;
};

class GraphBuilder {
public:
    bool addPass(const std::string& name, Pass *pass);
    bool connect(const std::string& src, const std::string& dst);
    // 위상 정렬과 입력 검사 후 실행 그래프를 만든다. 실패하면 nullptr.
    std::unique_ptr<Graph> compile();
private:
    static constexpr size_t npos = static_cast<size_t>(-1);

    inline size_t findPass(const std::string& name) const {
        auto it = passIndex.find(name);
        if(it != passIndex.end())
            return it->second;
        return npos;
    }

    std::vector<Pass*> passList;                         // addPass 순서 = 인덱스
    std::vector<std::string> passNames;                  // 같은 인덱스의 이름(오류·dump용)
    std::unordered_map<std::string, size_t> passIndex;   // 이름 → 인덱스
    std::vector<Edge> edges;

    //std::unordered_map<std::string, > inputs;
    //std::unordered_map<std::string, > outputs;
};

}
