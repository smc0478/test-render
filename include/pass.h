/*
 * 모든 Pass가 상속하는 베이스(GraphIr). 그래프가 Pass의 구체 타입을 몰라도 실행·필드 조회를 하게 한다.
 * (원래 graph-ir.h — 내용이 Pass 베이스라 pass.h로 변경, 기존 pass.h는 pass-node.h로)
 * - 있는 것: GraphIr(execute, id, 이름별 inputs/outputs 필드 맵), 필드 등록 함수 선언.
 * - 필요: 등록 함수 인자·접근 수준(PassField가 호출), 이름으로 필드 찾기, execute(PassContext&),
 *   가상 소멸자. 역할상 클래스 이름은 Pass가 더 맞을 수 있음.
 * - 범위: SDK 공개. 구현: src/pass.cc
 */
#pragma once

#include <unordered_map>
#include <string>


namespace RD {

class PassFieldBase;  // Forward Declaration

class Pass {
public:

    virtual void execute() = 0;
    void registerField(std::string name, PassFieldBase *field);

    PassFieldBase *findInput(const std::string &name) const;
    PassFieldBase *findOutput(const std::string &name) const;
    size_t inputCount() const { return inputs.size(); }

private:
    int id;
    std::unordered_map<std::string, PassFieldBase*> inputs;
    std::unordered_map<std::string, PassFieldBase*> outputs;
};

}
