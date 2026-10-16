/*
 * 1단계 3번 테스트: GraphBuilder로 Pass 추가·필드 연결(graph-builder.h).
 * 테스트가 가정하는 API (구현하면서 이름이 바뀌면 여기도 같이 고친다):
 * - addPass(name, Pass*) → 성공 여부. bool이나 explicit operator bool이 있는 Result 둘 다 된다.
 * - connect("src.field", "dst.field") → 성공 여부. 앞이 출발(출력 필드), 뒤가 도착(입력 필드).
 * - 빌더는 Pass를 소유하지 않는다(포인터만 보관).
 */
#include "graph-builder.h"
#include "test-passes.h"
#include "test.h"

namespace {

void addPassAndConnect() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    CHECK(g.addPass("c", &c));
    CHECK(g.addPass("a", &a));
    CHECK(g.connect("c.out", "a.in"));
}

void duplicatePassNameFails() {
    RD::GraphBuilder g;
    Constant c1;
    Constant c2;
    CHECK(g.addPass("c", &c1));
    CHECK(!g.addPass("c", &c2));
}

void nullPassFails() {
    RD::GraphBuilder g;
    CHECK(!g.addPass("x", nullptr));
}

void unknownPassOrFieldFails() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    g.addPass("c", &c);
    g.addPass("a", &a);
    CHECK(!g.connect("nope.out", "a.in"));
    CHECK(!g.connect("c.out", "nope.in"));
    CHECK(!g.connect("c.nope", "a.in"));
    CHECK(!g.connect("c.out", "a.nope"));
}

void wrongDirectionFails() {
    RD::GraphBuilder g;
    AddOne a1;
    AddOne a2;
    g.addPass("a1", &a1);
    g.addPass("a2", &a2);
    CHECK(!g.connect("a1.in", "a2.in"));    // 출발이 입력 필드
    CHECK(!g.connect("a1.out", "a2.out"));  // 도착이 출력 필드
    CHECK(!g.connect("a2.in", "a1.out"));   // 순서를 거꾸로 씀
}

void malformedPathFails() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    g.addPass("c", &c);
    g.addPass("a", &a);
    CHECK(!g.connect("cout", "a.in"));
    CHECK(!g.connect("c.", "a.in"));
    CHECK(!g.connect(".out", "a.in"));
    CHECK(!g.connect("", ""));
}

} // namespace

int main() {
    RUN(addPassAndConnect);
    RUN(duplicatePassNameFails);
    RUN(nullPassFails);
    RUN(unknownPassOrFieldFails);
    RUN(wrongDirectionFails);
    RUN(malformedPathFails);
    return TEST_RESULT();
}
