/*
 * Pass의 입출력 필드와 필드 선언 매크로를 정의하는 파일.
 * - FieldDirection: 입력과 출력 방향 구분.
 * - FieldBase: 공통 필드 인터페이스를 마련할 베이스 클래스.
 * - Field<T, D>: 소유 GraphIr, 필드 이름, 데이터 포인터와 등록 동작의 초안.
 * - InputField<T>, OutputField<T>: 방향별 필드 별칭.
 * - DECL_FIELD_IN, DECL_FIELD_OUT: Pass 멤버로 입출력 필드를 선언하는 매크로.
 * - 실제 데이터 접근 연산자와 GraphIr 등록 인터페이스 연동은 미완성이다.
 */
#pragma once

#include <string>
#include "graph-ir.h"

namespace RD {

enum class FieldDirection {
    Input,
    Output
};

class FieldBase {
public:
};

template <typename T, FieldDirection D>
class Field : public FieldBase {
public:
    Field(GraphIr *base, std::string name)
        : base(base), name(std::move(name)) {
        base->registerField(this->name, this);
    }

    FieldDirection direction() const override {
        return D;
    }
private:
    GraphIr *base;
    std::string name;
    T* data = nullptr;
};


template <typename T>
using InputField = Field<T, FieldDirection::Input>;

template <typename T>
using OutputField = Field<T, FieldDirection::Output>;

#define DECL_FIELD_IN(type, name) \
    RD::InputField<type> name{this, #name}

#define DECL_FIELD_OUT(type, name) \
    RD::OutputField<type> name{this, #name}

}