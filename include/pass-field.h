/*
 * Pass의 입출력 필드(포트)와 선언 매크로. DECL_FIELD_IN/OUT 한 줄로 포트를 만들고 소유 Pass에 자동 등록한다.
 * - PassFieldBase: 이름, 방향, 스키마(크기·정렬 포함), 데이터 포인터(void*). compile이 T를 몰라도 bind한다.
 * - PassField<T, D>: 입력은 const T(읽기 전용), 출력은 T. 날 메모리에 T를 얹으므로 trivially copyable만.
 * - 범위: SDK 공개.
 */
#pragma once

#include <string>
#include <type_traits>

#include "pass.h"
#include "schema-field.h"

namespace RD {

enum class PassFieldDirection {
    Input,
    Output
};

class PassFieldBase {
public:
    PassFieldDirection direction() const {
        return _direction;
    }
    const std::string& name() const { return _name; }
    const SchemaDesc &schema() const { return *_schema; }
    size_t size() const { return _schema->size(); }
    size_t align() const { return _schema->align(); }

    // compile이 T를 몰라도 저장 공간을 연결할 수 있게 void*로 받는다.
    void bind(void *data) { _data = data; }
    void *data() const { return _data; }

protected:
    PassFieldBase(std::string name, PassFieldDirection direction, const SchemaDesc *schema)
        : _name(std::move(name)), _direction(direction), _schema(schema) {}

    void *_data = nullptr;

private:
    std::string _name;
    PassFieldDirection _direction;
    const SchemaDesc *_schema;
};

template <typename T, PassFieldDirection D>
class PassField : public PassFieldBase {
    // 날 메모리에 T를 그대로 얹으므로 생성자·소멸자가 필요 없는 타입만 허용한다.
    static_assert(std::is_trivially_copyable_v<T>,
                  "pass field type must be trivially copyable");
public:
    using Value = std::conditional_t<D == PassFieldDirection::Input, const T, T>;

    PassField(Pass *base, std::string name)
        : PassFieldBase(std::move(name), D, &SchemaTraits<T>::desc()) {
        base->registerField(this->name(), this);
    }

    // 테스트·직접 연결용. 입력도 메모리는 T*로 받고, 읽기 전용은 접근할 때만 건다.
    void bind(T *data) { PassFieldBase::bind(data); }

    Value* operator->() const { return static_cast<Value*>(_data); }
    Value& operator*() const { return *static_cast<Value*>(_data); }
};


template <typename T>
using PassInputField = PassField<T, PassFieldDirection::Input>;

template <typename T>
using PassOutputField = PassField<T, PassFieldDirection::Output>;

#define DECL_FIELD_IN(type, name) \
    RD::PassInputField<type> name{this, #name}

#define DECL_FIELD_OUT(type, name) \
    RD::PassOutputField<type> name{this, #name}

}
