/*
 * Pass와 입출력 연결을 조립하는 그래프 빌더의 선언을 모은 파일.
 * - Endpoint: 그래프 입력, 그래프 출력, Pass 필드의 종류와 이름을 표현.
 * - Edge: 연결의 출발점과 도착점을 보관.
 * - GraphBuilder: Pass, 연결, 외부 입출력의 저장 공간과 추가 API 선언.
 * - validate, compile: 위상 정렬을 포함한 검증 및 컴파일의 구현 예정 위치.
 * - 그래프 입출력의 자료형과 각 API의 구현은 아직 완성하지 않았다.
 */
#ifndef _GRAPH_BUILDER_H_
#define _GRAPH_BUILDER_H_

#include "graph-ir.h"
#include "schema.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace RD {

class Endpoint {
public:
    enum class Kind {
        GraphInput,
        GraphOutput,
        GraphField
    };
private:
    Kind kind;
    std::string passName;
    std::string fieldName;
};

class Edge {
public:
private:
    Endpoint src;
    Endpoint dst;
};

class GraphBuilder {
public:
    void addPass();
    void connect();
    void addInput();
    void addOutput();
    // must topological sort
    void validate();
    void compile();
private:
    std::unordered_map<std::string, GraphIr*> passes;
    std::vector<Edge> edges;

    std::unordered_map<std::string, > inputs;
    std::unordered_map<std::string, > outputs;
};

}

#endif // graph-builder.h