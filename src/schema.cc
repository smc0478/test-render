/*
 * schema.h / schema-field.h 구현. 등록부·컴파일러 파일을 따로 두지 않고 여기 모은다(schema-plan.md).
 * - 구현할 것: 등록 후 고정(freeze), 키 버전 규칙.
 */
#include "schema.h"
#include "schema-field.h"

namespace RD {

SchemaDesc::SchemaDesc(std::string key, size_t size, size_t align,
                       std::vector<SchemaFieldDesc> fields)
    : _key(std::move(key)), _size(size), _align(align), _fields(std::move(fields)) {}

const SchemaFieldDesc *SchemaDesc::findField(const std::string &name) const {
    for(const SchemaFieldDesc &field : _fields)
        if(field.name == name)
            return &field;
    return nullptr;
}

bool compatible(const SchemaDesc &output, const SchemaDesc &input) {
    return output.key() == input.key() && output.size() == input.size();
}

const SchemaDesc *resolveFieldPath(const SchemaDesc &root, const std::string &path, size_t &offset) {
    const SchemaDesc *current = &root;
    size_t begin = 0;
    while(begin <= path.size()) {
        size_t dot = path.find('.', begin);
        size_t end = dot == std::string::npos ? path.size() : dot;
        const SchemaFieldDesc *field = current->findField(path.substr(begin, end - begin));
        if(field == nullptr)
            return nullptr;

        offset += field->offset;
        current = field->schema;
        if(dot == std::string::npos)
            return current;
        begin = dot + 1;
    }
    return nullptr;
}

static bool sameDefinition(const SchemaDesc &a, const SchemaDesc &b) {
    if(a.key() != b.key() || a.size() != b.size() || a.align() != b.align())
        return false;
    if(a.fields().size() != b.fields().size())
        return false;
    for(size_t i = 0; i < a.fields().size(); ++i) {
        const SchemaFieldDesc &fa = a.fields()[i];
        const SchemaFieldDesc &fb = b.fields()[i];
        if(fa.name != fb.name || fa.offset != fb.offset || fa.schema->key() != fb.schema->key())
            return false;
    }
    return true;
}

bool Schema::registerSchema(const SchemaDesc &desc) {
    auto it = _schemas.find(desc.key());
    if(it != _schemas.end())
        return sameDefinition(*it->second, desc);

    // 노출 필드의 스키마도 같이 등록한다.
    for(const SchemaFieldDesc &field : desc.fields())
        if(!registerSchema(*field.schema))
            return false;

    _schemas.emplace(desc.key(), &desc);
    return true;
}

const SchemaDesc *Schema::find(const std::string &key) const {
    auto it = _schemas.find(key);
    if(it != _schemas.end())
        return it->second;
    return nullptr;
}

}
