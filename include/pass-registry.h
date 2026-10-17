/*
 * Pass 종류 등록부. createPass("core.GBuffer", "gbuffer")처럼 타입 이름으로 Pass를 만들기 위해 둔다.
 * - PassRegistry: add<T>("타입"), addGraph<T>("타입"), create / createGraph. 내장 Pass와 DLL Pass가 같이 쓴다.
 *   DLL은 자기 static 등록부가 따로 생기므로 자동 등록 매크로 대신 호스트 등록부에 직접 add 한다.
 * - GraphPass: 실행 대신 expand(GraphBuilder&, 설정)로 Pass와 연결을 만드는 Pass(예: SponzaStylizedGraph).
 *   재구성 때만 호출하고 GPU 기록은 하지 않는다.
 * - GraphBuildContext: GraphPass가 읽는 조건(등록된 Pass 확인, bool 설정).
 * - 필요: 타입 키에 버전, Pass 생성·파괴를 같은 모듈에서(지금은 같은 CRT 가정), GraphPass 중첩.
 * - 범위: SDK 공개. 구현: src/pass-registry.cc
 */
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "pass.h"

namespace RD {

class GraphBuilder;
class PassRegistry;

class GraphBuildContext {
public:
    explicit GraphBuildContext(const PassRegistry *registry = nullptr) : _registry(registry) {}

    bool hasPass(const std::string &type) const;
    void set(const std::string &key, bool value) { _settings[key] = value; }
    bool getBool(const std::string &key, bool fallback) const;

private:
    const PassRegistry *_registry;
    std::unordered_map<std::string, bool> _settings;
};

class GraphPass {
public:
    virtual ~GraphPass() = default;
    virtual bool expand(GraphBuilder &builder, const GraphBuildContext &ctx) = 0;
};

using PassFactory = std::unique_ptr<Pass> (*)();
using GraphPassFactory = std::unique_ptr<GraphPass> (*)();

class PassRegistry {
public:
    template <typename T>
    bool add(const std::string &type) {
        return addFactory(type, []() -> std::unique_ptr<Pass> { return std::make_unique<T>(); });
    }

    template <typename T>
    bool addGraph(const std::string &type) {
        return addGraphFactory(type, []() -> std::unique_ptr<GraphPass> { return std::make_unique<T>(); });
    }

    bool addFactory(const std::string &type, PassFactory factory);
    bool addGraphFactory(const std::string &type, GraphPassFactory factory);

    std::unique_ptr<Pass> create(const std::string &type) const;
    std::unique_ptr<GraphPass> createGraph(const std::string &type) const;
    bool has(const std::string &type) const;

private:
    std::unordered_map<std::string, PassFactory> _passes;
    std::unordered_map<std::string, GraphPassFactory> _graphs;
};

}
