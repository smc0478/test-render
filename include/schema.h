/*
 * 렌더러의 타입 및 입출력 계약을 나타내는 스키마 초안을 모은 파일.
 * - SchemaDesc: 공통 타입 이름, 데이터 크기, 정렬 정보.
 * - ScalarDesc::Kind: Bool, Int32, Uint32, Float32 기본형 분류.
 * - TextureDesc: 텍스처 스키마를 확장할 파생 클래스의 자리.
 * - Schema: 이름별 스키마 정의를 보관하는 unordered_map 등록부의 초안.
 * - 등록, 조회, 호환성 검사 및 파생 타입을 보존하는 저장 방식은 구현 예정이다.
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