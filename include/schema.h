/*
 * 스키마 = 렌더러의 타입이자 in/out 연결 계약. 두 필드를 이어도 되는지 판단하는 기준과 등록부.
 * (schema-plan.md: 스키마는 schema.h / schema-field.h / schema.cc 세 파일로 유지)
 * - 있는 것: SchemaFieldDesc(노출 필드: 이름, 스키마, offset), SchemaDesc(키, size, align, 노출 필드),
 *   compatible(출력 → 입력), resolveFieldPath("a.b" → 스키마 + offset), Schema 등록부.
 * - 정체성은 키 문자열로 판단한다. 같은 타입이라도 DLL마다 static 객체 주소가 달라서다.
 * - 필요: 키에 버전, 등록 후 고정(freeze).
 * - 범위: SDK 공개.
 */
#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace RD {

class SchemaDesc;

// 노출 필드 하나. offset은 구조체 시작에서의 바이트 위치.
struct SchemaFieldDesc {
    std::string name;
    const SchemaDesc *schema;
    size_t offset;
};

class SchemaDesc {
public:
    SchemaDesc(std::string key, size_t size, size_t align,
               std::vector<SchemaFieldDesc> fields = {});

    const std::string &key() const { return _key; }
    size_t size() const { return _size; }
    size_t align() const { return _align; }
    const std::vector<SchemaFieldDesc> &fields() const { return _fields; }
    const SchemaFieldDesc *findField(const std::string &name) const;

private:
    std::string _key;
    size_t _size;
    size_t _align;
    std::vector<SchemaFieldDesc> _fields;
};

// 출력 스키마를 입력에 이어도 되는지. 지금은 키와 크기가 같으면 호환.
bool compatible(const SchemaDesc &output, const SchemaDesc &input);

// root의 노출 필드 경로("a" 또는 "a.b")를 따라가 마지막 필드의 스키마를 돌려주고 offset에 누적한다.
// 경로에 없는 이름이 있으면 nullptr.
const SchemaDesc *resolveFieldPath(const SchemaDesc &root, const std::string &path, size_t &offset);

// 스키마 등록부. 키 → 정의(비소유, 정의는 SchemaTraits의 static 객체).
class Schema {
public:
    // 같은 키가 다른 정의(크기·정렬·노출 필드)로 이미 있으면 false. 같은 정의면 true.
    bool registerSchema(const SchemaDesc &desc);
    const SchemaDesc *find(const std::string &key) const;
    size_t count() const { return _schemas.size(); }

private:
    std::unordered_map<std::string, const SchemaDesc *> _schemas;
};

}
