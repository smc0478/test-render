/*
 * graph-builder.h, graph.h 구현. connect는 연결을 기록만 하고, compile()이 위상 정렬(Kahn)·입력 검사 후
 * 출력 저장 공간을 만들어 bind한 Graph를 돌려준다.
 * 실패: 잘못된 연결, 순환, 연결 안 된 입력. 안 쓰는 출력·그래프 입력은 경고만.
 */
#include "graph-builder.h"
#include "pass-field.h"

#include <cstring>
#include <deque>
#include <new>
#include <sstream>
#include <unordered_set>

namespace RD {

// --- GraphBuilder ---

bool GraphBuilder::fail(const std::string& message) {
    _diagnostics.push_back({Severity::Error, message});
    return false;
}

void GraphBuilder::warn(const std::string& message) {
    _diagnostics.push_back({Severity::Warning, message});
}

bool GraphBuilder::addPass(const std::string& name, Pass *pass) {
    if(pass == nullptr)
        return fail("addPass: null pass '" + name + "'");
    // 점은 connect에서 Pass와 필드를 나누는 구분자라 이름에 쓸 수 없다.
    if(name.empty() || name.find('.') != std::string::npos)
        return fail("addPass: invalid pass name '" + name + "'");
    if(findPass(name) != npos)
        return fail("addPass: duplicate pass name '" + name + "'");

    passIndex.emplace(name, passList.size());
    passList.push_back(pass);
    passNames.push_back(name);
    return true;
}

Pass *GraphBuilder::createPass(const std::string& type, const std::string& name) {
    if(_registry == nullptr) {
        fail("createPass: no pass registry");
        return nullptr;
    }
    std::shared_ptr<Pass> pass = _registry->create(type);
    if(pass == nullptr) {
        fail("createPass: unknown pass type '" + type + "'");
        return nullptr;
    }
    if(!addPass(name, pass.get()))
        return nullptr;
    _owned.push_back(pass);
    return pass.get();
}

bool GraphBuilder::expand(const std::string& graphPassType, const GraphBuildContext& ctx) {
    if(_registry == nullptr)
        return fail("expand: no pass registry");
    std::unique_ptr<GraphPass> graphPass = _registry->createGraph(graphPassType);
    if(graphPass == nullptr)
        return fail("expand: unknown graph pass '" + graphPassType + "'");
    if(!graphPass->expand(*this, ctx))
        return fail("expand: '" + graphPassType + "' failed");
    return true;
}

bool GraphBuilder::addInput(const std::string& name, const SchemaDesc& schema) {
    if(name.empty() || !_inputs.emplace(name, &schema).second)
        return fail("addInput: invalid or duplicate graph input '" + name + "'");
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

// "pass.field" 또는 "pass.field.sub" → 출력 필드 끝점과 (하위 필드까지 내려간) 스키마
bool GraphBuilder::resolveSource(const std::string& path, Endpoint& endpoint, const SchemaDesc*& schema) {
    std::string passName, fieldPath;
    if(!splitFirst(path, passName, fieldPath))
        return fail("connect: '" + path + "' is not 'pass.field'");

    size_t index = findPass(passName);
    if(index == npos)
        return fail("connect: unknown pass '" + passName + "'");

    std::string fieldName = fieldPath, subPath;
    splitFirst(fieldPath, fieldName, subPath);

    PassFieldBase *field = passList[index]->findOutput(fieldName);
    if(field == nullptr)
        return fail("connect: '" + passName + "' has no output '" + fieldName + "'");

    endpoint = {index, fieldName, 0};
    schema = &field->schema();
    if(!subPath.empty()) {
        schema = resolveFieldPath(field->schema(), subPath, endpoint.offset);
        if(schema == nullptr)
            return fail("connect: '" + field->schema().key() + "' has no exposed field '" + subPath + "'");
    }
    return true;
}

// "pass.field" → 입력 필드 끝점. 입력은 하위 필드에 연결할 수 없다.
bool GraphBuilder::resolveTarget(const std::string& path, Endpoint& endpoint, PassFieldBase*& field) {
    std::string passName, fieldName;
    if(!splitFirst(path, passName, fieldName))
        return fail("connect: '" + path + "' is not 'pass.field'");

    size_t index = findPass(passName);
    if(index == npos)
        return fail("connect: unknown pass '" + passName + "'");

    if(fieldName.find('.') != std::string::npos)
        return fail("connect: input '" + path + "' cannot be a sub field");
    field = passList[index]->findInput(fieldName);
    if(field == nullptr)
        return fail("connect: '" + passName + "' has no input '" + fieldName + "'");

    endpoint = {index, fieldName, 0};
    return true;
}

bool GraphBuilder::connect(const std::string& src, const std::string& dst) {
    Endpoint srcEnd, dstEnd;
    const SchemaDesc *srcSchema = nullptr;
    PassFieldBase *dstField = nullptr;
    if(!resolveSource(src, srcEnd, srcSchema) || !resolveTarget(dst, dstEnd, dstField))
        return false;

    if(!compatible(*srcSchema, dstField->schema()))
        return fail("connect: type mismatch " + src + " (" + srcSchema->key() + ") -> " +
                    dst + " (" + dstField->schema().key() + ")");

    edges.push_back({srcEnd, dstEnd});
    return true;
}

bool GraphBuilder::connectInput(const std::string& input, const std::string& dst) {
    auto it = _inputs.find(input);
    if(it == _inputs.end())
        return fail("connectInput: unknown graph input '" + input + "'");

    Endpoint dstEnd;
    PassFieldBase *dstField = nullptr;
    if(!resolveTarget(dst, dstEnd, dstField))
        return false;

    if(!compatible(*it->second, dstField->schema()))
        return fail("connectInput: type mismatch " + input + " (" + it->second->key() + ") -> " +
                    dst + " (" + dstField->schema().key() + ")");

    _inputEdges.push_back({input, dstEnd});
    return true;
}

bool GraphBuilder::exportOutput(const std::string& name, const std::string& src) {
    for(const OutputRef &out : _outputs)
        if(out.name == name)
            return fail("exportOutput: duplicate graph output '" + name + "'");

    Endpoint srcEnd;
    const SchemaDesc *schema = nullptr;
    if(!resolveSource(src, srcEnd, schema))
        return false;

    _outputs.push_back({name, srcEnd});
    return true;
}

// Kahn 위상 정렬. sorted에 Pass 인덱스를 실행 순서로 담는다. 순환이면 false.
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

static std::shared_ptr<void> allocateAligned(size_t size, size_t align) {
    void *memory = ::operator new(size, std::align_val_t(align));
    std::memset(memory, 0, size);
    return std::shared_ptr<void>(memory, [align](void *p) { ::operator delete(p, std::align_val_t(align)); });
}

std::unique_ptr<Graph> GraphBuilder::compile() {
    std::vector<size_t> sorted;
    if(!topoSort(passList.size(), edges, sorted)) {
        std::vector<bool> placed(passList.size(), false);
        for(size_t index : sorted)
            placed[index] = true;
        std::string names;
        for(size_t i = 0; i < passList.size(); ++i)
            if(!placed[i])
                names += " " + passNames[i];
        fail("compile: cycle among passes:" + names);
        return nullptr;
    }

    std::vector<size_t> position(passList.size());  // Pass 인덱스 → 정렬 위치
    for(size_t i = 0; i < sorted.size(); ++i)
        position[sorted[i]] = i;

    std::unique_ptr<Graph> graph(new Graph());  // 생성자는 private, GraphBuilder는 friend
    graph->_nodes.resize(sorted.size());
    for(size_t i = 0; i < sorted.size(); ++i) {
        graph->_nodes[i].name = passNames[sorted[i]];
        graph->_nodes[i].pass = passList[sorted[i]];
    }

    // 입력 하나에 생산자는 하나(Pass 출력이든 그래프 입력이든).
    std::vector<std::unordered_set<std::string>> fromGraphInput(sorted.size());
    for(const Edge &e : edges) {
        PassWrap &node = graph->_nodes[position[e.dst.pass]];
        IrEdge link{position[e.src.pass], e.src.field, e.src.offset};
        if(!node.inputs.emplace(e.dst.field, link).second) {
            fail("compile: input '" + node.name + "." + e.dst.field + "' has more than one producer");
            return nullptr;
        }
    }
    for(const InputEdge &ie : _inputEdges) {
        size_t at = position[ie.dst.pass];
        PassWrap &node = graph->_nodes[at];
        if(node.inputs.count(ie.dst.field) || !fromGraphInput[at].insert(ie.dst.field).second) {
            fail("compile: input '" + node.name + "." + ie.dst.field + "' has more than one producer");
            return nullptr;
        }
    }

    // 연결 안 된 입력은 bind되지 않아 실행 중 빈 포인터를 읽으므로 실패다.
    for(size_t i = 0; i < graph->_nodes.size(); ++i) {
        const PassWrap &node = graph->_nodes[i];
        for(const auto &[fieldName, field] : node.pass->inputFields()) {
            (void)field;
            if(!node.inputs.count(fieldName) && !fromGraphInput[i].count(fieldName)) {
                fail("compile: input '" + node.name + "." + fieldName + "' is not connected");
                return nullptr;
            }
        }
    }

    // 경고: 아무도 읽지 않는 출력, 연결 안 된 그래프 입력.
    std::unordered_set<std::string> usedOutputs;
    for(const Edge &e : edges)
        usedOutputs.insert(passNames[e.src.pass] + "." + e.src.field);
    for(const OutputRef &out : _outputs)
        usedOutputs.insert(passNames[out.src.pass] + "." + out.src.field);
    for(const PassWrap &node : graph->_nodes)
        for(const auto &[fieldName, field] : node.pass->outputFields()) {
            (void)field;
            if(!usedOutputs.count(node.name + "." + fieldName))
                warn("compile: output '" + node.name + "." + fieldName + "' is not used");
        }
    for(const auto &[inputName, schema] : _inputs) {
        (void)schema;
        bool used = false;
        for(const InputEdge &ie : _inputEdges)
            used = used || ie.input == inputName;
        if(!used)
            warn("compile: graph input '" + inputName + "' is not connected");
    }

    // bind: 앞에서부터 출력 저장 공간을 만들고, 입력은 생산자의 출력(+ 하위 필드 offset)에 연결한다.
    // 생산자는 항상 앞 노드라 입력을 연결할 때 저장 공간이 이미 있다.
    for(PassWrap &node : graph->_nodes) {
        for(const auto &[fieldName, field] : node.pass->outputFields()) {
            (void)fieldName;
            std::shared_ptr<void> block = allocateAligned(field->size(), field->align());
            field->bind(block.get());
            graph->_storage.push_back(std::move(block));
        }
        for(const auto &[fieldName, link] : node.inputs) {
            PassFieldBase *producer = graph->_nodes[link.producer].pass->findOutput(link.outputField);
            node.pass->findInput(fieldName)->bind(static_cast<char *>(producer->data()) + link.offset);
        }
    }
    for(const InputEdge &ie : _inputEdges) {
        PassFieldBase *field = passList[ie.dst.pass]->findInput(ie.dst.field);
        field->bind(nullptr);
        graph->_inputBindings[ie.input].fields.push_back(field);
    }
    for(const OutputRef &out : _outputs) {
        PassFieldBase *field = passList[out.src.pass]->findOutput(out.src.field);
        graph->_outputs[out.name] = static_cast<char *>(field->data()) + out.src.offset;
    }

    graph->_owned = _owned;
    return graph;
}

// --- Graph ---

void Graph::execute() {
    for(PassWrap &node : _nodes)
        node.pass->execute();
}

void Graph::prepare(PassContext &ctx) {
    for(PassWrap &node : _nodes)
        node.pass->prepare(ctx);
}

void Graph::record(PassContext &ctx) {
    for(PassWrap &node : _nodes)
        node.pass->record(ctx);
}

bool Graph::bindInput(const std::string &name, void *data) {
    auto it = _inputBindings.find(name);
    if(it == _inputBindings.end())
        return false;
    for(PassFieldBase *field : it->second.fields)
        field->bind(data);
    it->second.bound = data != nullptr;
    return true;
}

bool Graph::inputsBound() const {
    for(const auto &[name, binding] : _inputBindings) {
        (void)name;
        if(!binding.bound)
            return false;
    }
    return true;
}

const void *Graph::output(const std::string &name) const {
    auto it = _outputs.find(name);
    if(it != _outputs.end())
        return it->second;
    return nullptr;
}

std::string Graph::dump() const {
    std::ostringstream out;
    for(size_t i = 0; i < _nodes.size(); ++i) {
        const PassWrap &node = _nodes[i];
        out << i << " " << node.name;
        for(const auto &[fieldName, link] : node.inputs)
            out << "  " << fieldName << " <- " << _nodes[link.producer].name << "." << link.outputField;
        out << "\n";
    }
    for(const auto &[name, binding] : _inputBindings)
        out << "input " << name << " -> " << binding.fields.size() << " field(s)\n";
    return out.str();
}

}
