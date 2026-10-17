/*
 * 실행 가능한 그래프 = GraphBuilder::compile()의 결과. 런타임은 이 단위로 실행·교체한다.
 * GPU 핸들은 넣지 않는다(GPU 자원은 Pass가 prepare에서 만들어 필드 값으로 넘긴다).
 * - 있는 것: PassWrap(이름, Pass, 입력 필드별 생산자), IrEdge, Graph(위상 정렬된 노드, 출력 저장 공간,
 *   그래프 입력 bind, 그래프 출력 조회, execute / prepare / record, 텍스트 dump).
 *   생성은 GraphBuilder::compile()만(friend).
 * - 필요: revision, 그래프 교체 시 이전 그래프 지연 해제.
 * - 범위: 엔진 내부. 구현: src/graph-builder.cc
 */
#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include "pass.h"

namespace RD {

class GraphBuilder;
class PassContext;
class PassFieldBase;

// 입력 하나를 채우는 생산자. producer는 Graph 노드 인덱스이고 항상 자기 노드보다 앞이다.
struct IrEdge {
    size_t producer;
    std::string outputField;
    size_t offset = 0;  // 하위 필드("out.x")면 출력 안에서의 위치
};

struct PassWrap {
    std::string name;
    Pass *pass = nullptr;
    // 내 입력 필드 이름 → 생산자. 키가 입력 필드라 입력 하나에 생산자는 하나뿐이다.
    // 그래프 입력에서 오는 필드는 여기 없고 Graph::bindInput으로 연결된다.
    std::unordered_map<std::string, IrEdge> inputs;
};

class Graph {
public:
    void execute();                // CPU 실행
    void prepare(PassContext &ctx);
    void record(PassContext &ctx);

    // 그래프 입력에 바깥 데이터를 연결한다. 매 프레임 다시 불러도 된다.
    bool bindInput(const std::string &name, void *data);
    template <typename T>
    bool bindInput(const std::string &name, T *data) { return bindInput(name, static_cast<void *>(data)); }
    bool inputsBound() const;

    // exportOutput으로 내보낸 값의 위치.
    const void *output(const std::string &name) const;
    template <typename T>
    const T *output(const std::string &name) const { return static_cast<const T *>(output(name)); }

    const std::vector<PassWrap> &nodes() const { return _nodes; }
    std::string dump() const;

private:
    friend class GraphBuilder;
    Graph() = default;

    struct InputBinding {
        std::vector<PassFieldBase *> fields;
        bool bound = false;
    };

    std::vector<PassWrap> _nodes;                 // 위상 정렬 순서
    std::vector<std::shared_ptr<void>> _storage;  // 출력 필드 저장 공간
    std::vector<std::shared_ptr<Pass>> _owned;    // createPass로 만든 Pass
    std::unordered_map<std::string, InputBinding> _inputBindings;
    std::unordered_map<std::string, void *> _outputs;
};

}
