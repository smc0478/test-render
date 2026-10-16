/*
 * 1단계 1번 테스트: Pass 필드 선언·등록(pass.h, pass-field.h). 데이터 접근은 test-pass-data.cc.
 * 테스트가 가정하는 API (구현하면서 이름이 바뀌면 여기도 같이 고친다):
 * - Pass: 가상 소멸자, 복사 금지(필드가 자기 주소를 등록하므로), findInput(name) / findOutput(name)
 *   → PassFieldBase*(없으면 nullptr).
 * - PassFieldBase: name(), direction().
 */
#include "pass-field.h"
#include "test-passes.h"
#include "test.h"

#include <string>
#include <type_traits>

static_assert(std::has_virtual_destructor_v<RD::Pass>,
              "Pass needs a virtual destructor");
static_assert(!std::is_copy_constructible_v<AddOne> && !std::is_copy_assignable_v<AddOne>,
              "copying a Pass would leave fields registered at the old object's address");

namespace {

void fieldsAreRegistered() {
    AddOne p;
    RD::PassFieldBase *in = p.findInput("in");
    RD::PassFieldBase *out = p.findOutput("out");
    CHECK(in != nullptr);
    CHECK(out != nullptr);
    if (!in || !out) {
        return;
    }
    CHECK_EQ(std::string(in->name()), std::string("in"));
    CHECK_EQ(std::string(out->name()), std::string("out"));
    CHECK(in->direction() == RD::PassFieldDirection::Input);
    CHECK(out->direction() == RD::PassFieldDirection::Output);
}

void registeredPointerIsTheMember() {
    AddOne p;
    CHECK(p.findInput("in") == &p.in);
    CHECK(p.findOutput("out") == &p.out);
}

void lookupRespectsDirection() {
    AddOne p;
    CHECK(p.findInput("out") == nullptr);
    CHECK(p.findOutput("in") == nullptr);
    CHECK(p.findInput("missing") == nullptr);
    CHECK(p.findOutput("missing") == nullptr);
}

void eachInstanceHasOwnFields() {
    AddOne a;
    AddOne b;
    CHECK(a.findInput("in") == &a.in);
    CHECK(b.findInput("in") == &b.in);
    CHECK(a.findInput("in") != b.findInput("in"));
}

void multipleInputs() {
    Add p;
    CHECK(p.findInput("a") == &p.a);
    CHECK(p.findInput("b") == &p.b);
    CHECK(p.findOutput("sum") == &p.sum);
}

} // namespace

int main() {
    RUN(fieldsAreRegistered);
    RUN(registeredPointerIsTheMember);
    RUN(lookupRespectsDirection);
    RUN(eachInstanceHasOwnFields);
    RUN(multipleInputs);
    return TEST_RESULT();
}
