/*
 * graph-builder.h 구현. connect는 연결을 기록만 하고, compile()이 위상 정렬(Kahn)·입력 검사 후
 * Graph를 만든다. 실패: 잘못된 연결, 순환, 연결 안 된 입력. 안 쓰는 노드·출력은 경고만.
 * - 구현할 것: 연결 안 된 입력 실패, 경고, bind(출력 저장 공간), 그래프 입력/출력,
 *   디버그용 텍스트 dump.
 */
#include "graph-builder.h"
#include "pass-field.h"

#include <deque>

namespace RD {

bool GraphBuilder::addPass(const std::string& name, Pass *pass) {
    // 점은 connect에서 Pass와 필드를 나누는 구분자라 이름에 쓸 수 없다.
    if(pass == nullptr || name.empty() || name.find('.') != std::string::npos)
        return false;
    if(findPass(name) != npos)
        return false;

    passIndex.emplace(name, passList.size());
    passList.push_back(pass);
    passNames.push_back(name);
    return true;
}

// path.split('.', 1): "a.b.c" → "a", "b.c". 점이 없으면 false.
static bool splitFirst(const std::string &path,
                       std::string &head,
                       std::string &rest) {
    size_t dot = path.find('.');
    if(dot == std::string::npos)
        return false;

    head = path.substr(0, dot);
    rest = path.substr(dot + 1);
    return true;
}

bool GraphBuilder::connect(const std::string& src, const std::string& dst) {
    std::string srcPassName, srcFieldPath;
    std::string dstPassName, dstFieldName;
    if(!splitFirst(src, srcPassName, srcFieldPath) ||
       !splitFirst(dst, dstPassName, dstFieldName))
        return false;

    size_t srcIndex = findPass(srcPassName);
    if(srcIndex == npos)
        return false;

    size_t dstIndex = findPass(dstPassName);
    if(dstIndex == npos)
        return false;

    if(dstFieldName.find('.') != std::string::npos)
        return false;
    PassFieldBase *dstField = passList[dstIndex]->findInput(dstFieldName);
    if(dstField == nullptr)
        return false;
    // TODO: 하위 필드("edge.mask")는 스키마(5번) 이후. 지금은 Pass 필드까지만.
    if(srcFieldPath.find('.') != std::string::npos)
        return false;
    PassFieldBase *srcField = passList[srcIndex]->findOutput(srcFieldPath);
    if(srcField == nullptr)
        return false;

    // TODO: 스키마(5번)가 생기면 타입 호환 검사로 바꾼다. 지금은 크기만 비교.
    if(srcField->size() != dstField->size())
        return false;

    edges.push_back({{srcIndex, srcFieldPath}, {dstIndex, dstFieldName}});
    return true;
}

// Kahn 위상 정렬. sorted에 passList 인덱스를 실행 순서로 담는다. 순환이면 false.
static bool topoSort(size_t passCount, const std::vector<Edge> &edges,
                     std::vector<size_t> &sorted) {
    std::vector<int> indegree(passCount, 0);
    std::vector<std::vector<size_t>> next(passCount);
    for(const Edge &e : edges) {
        next[e.src.pass].push_back(e.dst.pass);
        indegree[e.dst.pass]++;
    }

    std::deque<size_t> ready;
    for(size_t i = 0; i < passCount; ++i)
        if(indegree[i] == 0)
            ready.push_back(i);  // addPass 순서 유지 → 같은 그래프는 항상 같은 순서

    while(!ready.empty()) {
        size_t n = ready.front();
        ready.pop_front();
        sorted.push_back(n);
        for(size_t m : next[n])
            if(--indegree[m] == 0)
                ready.push_back(m);
    }
    return sorted.size() == passCount;  // 다 못 꺼내면 순환
}

std::unique_ptr<Graph> GraphBuilder::compile() {
    std::vector<size_t> sorted;
    if(!topoSort(passList.size(), edges, sorted))
        return nullptr;  // 순환

    std::vector<size_t> position(passList.size());  // passList 인덱스 → 정렬 위치
    for(size_t i = 0; i < sorted.size(); ++i)
        position[sorted[i]] = i;

    std::unique_ptr<Graph> graph(new Graph());  // 생성자는 private, GraphBuilder는 friend
    graph->nodes.resize(sorted.size());
    for(size_t i = 0; i < sorted.size(); ++i)
        graph->nodes[i].pass = passList[sorted[i]];

    for(const Edge &e : edges) {
        PassWrap &node = graph->nodes[position[e.dst.pass]];
        IrEdge link{position[e.src.pass], e.src.field};
        if(!node.inputs.emplace(e.dst.field, link).second)
            return nullptr;  // 입력 하나에 생산자 둘
    }

    // TODO: 실패 — 연결 안 된 입력이 하나라도 있는 노드(node.inputs.size() != node.pass->inputCount()).
    //       bind되지 않아 execute에서 빈 포인터를 읽는다.
    // TODO: 경고 — 아무도 읽지 않는 출력, 어디에도 연결 안 된 노드. 그래프는 막지 않는다.

    // TODO: bind — 앞에서부터 출력 저장 공간을 만들고, 입력은 nodes[producer]의 출력에 연결
    return graph;
}

}
