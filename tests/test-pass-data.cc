/*
 * 1단계 2번 테스트: 필드로 데이터 접근(pass-field.h). bind를 구현한 뒤에 빌드한다.
 * 테스트가 가정하는 API (구현하면서 이름이 바뀌면 여기도 같이 고친다):
 * - PassField<T>: bind(Value*)로 데이터 연결(테스트와 GraphBuilder::compile이 사용), operator* / operator->.
 * - 입력 필드는 읽기 전용(const T)이라 bind에 T*를 넘겨도 된다.
 */
#include "pass-field.h"
#include "test-passes.h"
#include "test.h"

namespace {

struct Vec2 {
    int x;
    int y;
};

// 구조체 필드의 멤버 접근(in->x) 확인용.
struct SwapXY : public RD::Pass {
    DECL_FIELD_IN(Vec2, in);
    DECL_FIELD_OUT(Vec2, out);
    void execute() override {
        out->x = in->y;
        out->y = in->x;
    }
};

void dataAccessThroughFields() {
    AddOne p;
    int src = 41;
    int dst = 0;
    p.in.bind(&src);
    p.out.bind(&dst);
    p.execute();
    CHECK_EQ(dst, 42);
}

void structMemberAccess() {
    SwapXY p;
    Vec2 src{1, 2};
    Vec2 dst{0, 0};
    p.in.bind(&src);
    p.out.bind(&dst);
    p.execute();
    CHECK_EQ(dst.x, 2);
    CHECK_EQ(dst.y, 1);
}

void executeThroughBasePointer() {
    AddOne p;
    int src = 1;
    int dst = 0;
    p.in.bind(&src);
    p.out.bind(&dst);
    RD::Pass *base = &p;
    base->execute();
    CHECK_EQ(dst, 2);
}

} // namespace

int main() {
    RUN(dataAccessThroughFields);
    RUN(structMemberAccess);
    RUN(executeThroughBasePointer);
    return TEST_RESULT();
}
