/*
 * 그래프 조립 API. 호스트와 GraphPass 플러그인이 같은 API로 Pass를 추가하고 필드를 연결한다.
 * - 있는 것: Endpoint(Pass 인덱스 + 필드 + 하위 필드 offset), Edge, GraphBuilder
 *   (addPass / createPass / expand, addInput / connect / connectInput / exportOutput, compile, diagnostics).
 * - 경로: "pass.field" 또는 출력의 하위 필드 "pass.field.sub". 입력(dst)은 항상 필드 전체.
 * - Pass 인스턴스 하나는 살아 있는 그래프 하나에만 쓴다(compile이 필드를 bind하므로).
 * - 필요: 실패 이유를 Result로 돌려주기, GraphPass 중첩(하위 그래프 경계 포트).
 * - 범위: SDK 공개. 구현: src/graph-builder.cc
 */
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "core.h"
#include "pass.h"
#include "pass-registry.h"
#include "schema-field.h"
#include "graph.h"

namespace RD {

// 연결 한쪽 끝. pass는 GraphBuilder 안의 Pass 인덱스.
struct Endpoint {
    size_t pass;
    std::string field;
    size_t offset = 0;  // src 하위 필드의 offset(dst는 항상 0)
};

struct Edge {
    Endpoint src;
    Endpoint dst;
};

class GraphBuilder {
public:
    explicit GraphBuilder(const PassRegistry *registry = nullptr) : _registry(registry) {}

    // 바깥에서 만든 Pass를 추가한다(소유하지 않음).
    bool addPass(const std::string& name, Pass *pass);
    // 등록부에서 타입으로 만든다. 만든 Pass는 compile 결과 Graph가 같이 소유한다.
    Pass *createPass(const std::string& type, const std::string& name);
    // GraphPass를 이 빌더에 전개한다(GraphPass가 Pass와 연결을 추가).
    bool expand(const std::string& graphPassType, const GraphBuildContext& ctx);

    template <typename T>
    bool addInput(const std::string& name) { return addInput(name, SchemaTraits<T>::desc()); }
    bool addInput(const std::string& name, const SchemaDesc& schema);

    bool connect(const std::string& src, const std::string& dst);
    bool connectInput(const std::string& input, const std::string& dst);
    bool exportOutput(const std::string& name, const std::string& src);

    // 위상 정렬·입력 검사·bind 후 실행 그래프를 만든다. 실패하면 nullptr(이유는 diagnostics).
    // 실패: 잘못된 연결, 순환, 연결 안 된 입력. 안 쓰는 출력·그래프 입력은 경고만.
    std::unique_ptr<Graph> compile();

    const std::vector<Diagnostic>& diagnostics() const { return _diagnostics; }

private:
    static constexpr size_t npos = static_cast<size_t>(-1);

    struct InputEdge {
        std::string input;
        Endpoint dst;
    };
    struct OutputRef {
        std::string name;
        Endpoint src;
    };

    inline size_t findPass(const std::string& name) const {
        auto it = passIndex.find(name);
        if(it != passIndex.end())
            return it->second;
        return npos;
    }

    bool fail(const std::string& message);
    void warn(const std::string& message);
    bool resolveSource(const std::string& path, Endpoint& endpoint, const SchemaDesc*& schema);
    bool resolveTarget(const std::string& path, Endpoint& endpoint, PassFieldBase*& field);

    const PassRegistry *_registry;

    std::vector<Pass*> passList;                         // addPass 순서 = 인덱스
    std::vector<std::string> passNames;                  // 같은 인덱스의 이름(오류·dump용)
    std::unordered_map<std::string, size_t> passIndex;   // 이름 → 인덱스
    std::vector<std::shared_ptr<Pass>> _owned;           // createPass로 만든 Pass
    std::vector<Edge> edges;

    std::unordered_map<std::string, const SchemaDesc*> _inputs;  // 그래프 입력 이름 → 스키마
    std::vector<InputEdge> _inputEdges;
    std::vector<OutputRef> _outputs;

    std::vector<Diagnostic> _diagnostics;
};

}
