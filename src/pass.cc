/*
 * pass.h(GraphIr) 구현. 모든 Pass가 공유하는 필드 등록·조회. (원래 graph-ir.cc)
 * - 구현할 것: 필드 등록(이름 중복 검사), 이름으로 필드 조회, id 발급 방식 결정.
 */
#include "pass.h"
#include "pass-field.h"

namespace RD {

void Pass::registerField(std::string name, PassFieldBase *field) {
    auto &map = field->direction() == PassFieldDirection::Input ? inputs : outputs;
    map.emplace(std::move(name), field);
}

PassFieldBase *Pass::findInput(const std::string &name) const {
    auto it = inputs.find(name);
    if(it != inputs.end())
        return it->second;
    return nullptr;
}
PassFieldBase *Pass::findOutput(const std::string &name) const {
    auto it = outputs.find(name);
    if(it != outputs.end())
        return it->second;
    return nullptr;
}

}