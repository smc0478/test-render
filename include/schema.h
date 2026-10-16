/*
 * 스키마 = 렌더러의 타입이자 in/out 연결 계약. 두 필드를 이어도 되는지 판단하는 기준과 등록부.
 * (schema-plan.md: 스키마는 schema.h / schema-field.h / schema.cc 세 파일로 유지)
 * - 있는 것: SchemaDesc(이름, size, align), ScalarDesc::Kind, TextureDesc 자리, Schema 등록부 초안.
 * - 필요: 키에 버전, size/align은 sizeof/alignof, 파생 descriptor가 안 잘리게 포인터 소유,
 *   등록·조회·중복 검사·고정, 출력 → 입력 호환성 검사.
 * - 범위: SDK 공개.
 */
#pragma once

#include <string>
#include <unordered_map>


namespace RD {

class SchemaDesc {
private:
    // schema key?
    std::string name; // schema key;
    unsigned int size;
    unsigned int align;
};

class ScalarDesc {

    enum class Kind {
        Bool,
        Int32,
        Uint32,
        Float32,
    };

};

class TextureDesc : public SchemaDesc {

};

class Schema {
public:

private:
    std::string name;
    std::unordered_map<std::string, SchemaDesc> schemas;
};

}