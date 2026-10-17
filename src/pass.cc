/*
 * pass.h 구현. 모든 Pass가 공유하는 필드 등록·조회.
 */
#include "pass.h"
#include "pass-field.h"

#include <cassert>

namespace RD {

void Pass::registerField(std::string name, PassFieldBase *field) {
    auto &map = field->direction() == PassFieldDirection::Input ? inputs : outputs;
    bool added = map.emplace(std::move(name), field).second;
    assert(added && "same field name declared twice in one pass");
    (void)added;
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
