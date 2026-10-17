/*
 * 2단계 5번 테스트: 스키마(schema.h, schema-field.h)와 스키마로 하는 연결 검사·하위 필드 연결.
 * - 기본형 키, DEFINE_STRUCT_SCHEM의 노출 필드·offset, 중첩 경로, 등록부 충돌,
 *   타입이 다른 연결 실패, "pass.out.inner.b" 하위 필드 연결.
 */
#include "graph-builder.h"
#include "pass-field.h"
#include "schema-field.h"
#include "test-passes.h"
#include "test.h"

#include <cstddef>
#include <string>

struct Settings {
    uint32_t quality;
    float scale;
    bool shadows;
};
DEFINE_STRUCT_SCHEM(Settings, (quality, shadows))

struct Inner {
    int a;
    int b;
};
DEFINE_STRUCT_SCHEM(Inner, (a, b))

struct Outer {
    float pad;
    Inner inner;
};
DEFINE_STRUCT_SCHEM(Outer, (pad, inner))

namespace {

struct MakeOuter : public RD::Pass {
    DECL_FIELD_OUT(Outer, out);
    void execute() override {
        out->pad = 1.5f;
        out->inner = {7, 9};
    }
};

struct ReadFloat : public RD::Pass {
    DECL_FIELD_IN(float, in);
    float result = 0;
    void execute() override { result = *in; }
};

void basicKeys() {
    CHECK_EQ(RD::SchemaTraits<int>::desc().key(), std::string("int32"));
    CHECK_EQ(RD::SchemaTraits<float>::desc().key(), std::string("float32"));
    CHECK_EQ(RD::SchemaTraits<float>::desc().size(), sizeof(float));
}

void structFieldsAndOffsets() {
    const RD::SchemaDesc &desc = RD::SchemaTraits<Settings>::desc();
    CHECK_EQ(desc.key(), std::string("Settings"));
    CHECK_EQ(desc.size(), sizeof(Settings));
    CHECK_EQ(desc.fields().size(), size_t(2));
    const RD::SchemaFieldDesc *shadows = desc.findField("shadows");
    CHECK(shadows != nullptr);
    if(shadows) {
        CHECK_EQ(shadows->offset, offsetof(Settings, shadows));
        CHECK_EQ(shadows->schema->key(), std::string("bool"));
    }
    CHECK(desc.findField("scale") == nullptr);  // 노출하지 않은 필드
}

void nestedPath() {
    size_t offset = 0;
    const RD::SchemaDesc *s = RD::resolveFieldPath(RD::SchemaTraits<Outer>::desc(), "inner.b", offset);
    CHECK(s != nullptr && s->key() == "int32");
    CHECK_EQ(offset, offsetof(Outer, inner) + offsetof(Inner, b));

    size_t unused = 0;
    CHECK(RD::resolveFieldPath(RD::SchemaTraits<Outer>::desc(), "inner.c", unused) == nullptr);
    CHECK(RD::resolveFieldPath(RD::SchemaTraits<Outer>::desc(), "nope", unused) == nullptr);
}

void opaqueTypesDiffer() {
    struct A { int x; };
    struct B { int x; };
    const RD::SchemaDesc &a = RD::SchemaTraits<A>::desc();
    const RD::SchemaDesc &b = RD::SchemaTraits<B>::desc();
    CHECK(a.key() != b.key());
    CHECK(!RD::compatible(a, b));
    CHECK(RD::compatible(a, a));
}

void registryConflict() {
    RD::Schema registry;
    CHECK(registry.registerSchema(RD::SchemaTraits<Settings>::desc()));
    CHECK(registry.registerSchema(RD::SchemaTraits<Settings>::desc()));  // 같은 정의는 다시 등록해도 됨
    CHECK(registry.find("uint32") != nullptr);                          // 노출 필드 스키마도 등록됨

    static const RD::SchemaDesc fake("Settings", 1, 1);
    CHECK(!registry.registerSchema(fake));
}

void typeMismatchFails() {
    RD::GraphBuilder g;
    Constant c;
    ReadFloat r;
    g.addPass("c", &c);
    g.addPass("r", &r);
    CHECK(!g.connect("c.out", "r.in"));
    CHECK(!g.diagnostics().empty());
}

void subFieldConnect() {
    RD::GraphBuilder g;
    MakeOuter m;
    Capture capB;
    ReadFloat pad;
    g.addPass("m", &m);
    g.addPass("capB", &capB);
    g.addPass("pad", &pad);
    CHECK(g.connect("m.out.inner.b", "capB.in"));
    CHECK(g.connect("m.out.pad", "pad.in"));

    auto graph = g.compile();
    CHECK(graph);
    if(!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(capB.result, 9);
    CHECK_EQ(pad.result, 1.5f);
}

void badSubFieldFails() {
    RD::GraphBuilder g;
    MakeOuter m;
    Capture cap;
    g.addPass("m", &m);
    g.addPass("cap", &cap);
    CHECK(!g.connect("m.out.nope", "cap.in"));
    CHECK(!g.connect("m.out.inner", "cap.in"));    // Inner → int: 타입이 다름
    CHECK(!g.connect("m.out.inner.b", "cap.in.x"));  // 입력은 하위 필드 불가
}

} // namespace

int main() {
    RUN(basicKeys);
    RUN(structFieldsAndOffsets);
    RUN(nestedPath);
    RUN(opaqueTypesDiffer);
    RUN(registryConflict);
    RUN(typeMismatchFails);
    RUN(subFieldConnect);
    RUN(badSubFieldFails);
    return TEST_RESULT();
}
