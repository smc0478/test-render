/*
 * 모든 Pass가 상속하는 베이스. 그래프가 Pass의 구체 타입을 몰라도 실행·필드 조회를 하게 한다.
 * - 있는 것: Pass(execute / prepare / record, 이름별 inputs/outputs 필드 맵, 필드 등록·조회).
 *   CPU Pass는 execute()를, GPU Pass는 prepare(ctx)와 record(ctx)를 쓴다.
 *   필드가 자기 주소를 등록하므로 복사할 수 없다.
 * - 범위: SDK 공개. 구현: src/pass.cc
 */
#pragma once

#include <unordered_map>
#include <string>

namespace RD {

class PassFieldBase;  // Forward Declaration
class PassContext;

class Pass {
public:
    Pass() = default;
    Pass(const Pass &) = delete;
    Pass &operator=(const Pass &) = delete;
    virtual ~Pass() = default;

    // CPU 실행(테스트·CPU Pass).
    virtual void execute() {}
    // 그래프 준비 때 한 번: GPU 자원을 만들고 출력 필드에 핸들을 쓴다.
    virtual void prepare(PassContext &) {}
    // 매 프레임 GPU 명령 기록. 기본은 execute().
    virtual void record(PassContext &) { execute(); }

    void registerField(std::string name, PassFieldBase *field);

    PassFieldBase *findInput(const std::string &name) const;
    PassFieldBase *findOutput(const std::string &name) const;
    size_t inputCount() const { return inputs.size(); }
    const std::unordered_map<std::string, PassFieldBase*> &inputFields() const { return inputs; }
    const std::unordered_map<std::string, PassFieldBase*> &outputFields() const { return outputs; }

private:
    std::unordered_map<std::string, PassFieldBase*> inputs;
    std::unordered_map<std::string, PassFieldBase*> outputs;
};

}
