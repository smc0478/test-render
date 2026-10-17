/*
 * 1단계 4번 테스트: compile()로 실행 그래프를 만들고 CPU에서 실행(graph-builder.cc의 compile, graph.h).
 * 테스트가 가정하는 API (구현하면서 이름이 바뀌면 여기도 같이 고친다):
 * - GraphBuilder::compile() → 그래프 또는 실패. bool로 성공을 판정하고 -> 로 Graph에 접근한다
 *   (std::unique_ptr<Graph>, std::optional<Graph>, Result<Graph> 아무거나 된다).
 * - Graph::execute(): 위상 정렬 순서대로 모든 Pass의 execute()를 부른다. 여러 번 불러도 된다(매 프레임).
 * - compile()은 출력 필드마다 저장 공간을 만들고, 입력 필드를 연결된 출력에 bind한다.
 * - 실패: 잘못된 연결(입력 하나에 생산자 둘), 순환, 연결 안 된 입력. 안 쓰는 노드·출력은 경고만.
 */
#include "graph-builder.h"
#include "graph.h"
#include "pass-context.h"

#include <cstdint>
#include "test-passes.h"
#include "test.h"

namespace {

void chain() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    Capture out;
    c.value = 1;
    g.addPass("c", &c);
    g.addPass("a", &a);
    g.addPass("out", &out);
    g.connect("c.out", "a.in");
    g.connect("a.out", "out.in");

    auto graph = g.compile();
    CHECK(graph);
    if (!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(out.result, 2);
}

// 추가 순서가 실행 순서가 아니어야 한다(위상 정렬).
void addOrderDoesNotMatter() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    Capture out;
    c.value = 1;
    g.addPass("out", &out);
    g.addPass("a", &a);
    g.addPass("c", &c);
    g.connect("a.out", "out.in");
    g.connect("c.out", "a.in");

    auto graph = g.compile();
    CHECK(graph);
    if (!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(out.result, 2);
}

// 출력 하나를 여러 입력이 읽는다.
void fanOut() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a1;
    AddOne a2;
    Capture out1;
    Capture out2;
    c.value = 5;
    g.addPass("c", &c);
    g.addPass("a1", &a1);
    g.addPass("a2", &a2);
    g.addPass("out1", &out1);
    g.addPass("out2", &out2);
    g.connect("c.out", "a1.in");
    g.connect("c.out", "a2.in");
    g.connect("a1.out", "out1.in");
    g.connect("a2.out", "out2.in");

    auto graph = g.compile();
    CHECK(graph);
    if (!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(out1.result, 6);
    CHECK_EQ(out2.result, 6);
}

// 입력 두 개가 한 Pass로 합류한다.
void join() {
    RD::GraphBuilder g;
    Constant c1;
    Constant c2;
    Add add;
    Capture out;
    c1.value = 2;
    c2.value = 3;
    g.addPass("c1", &c1);
    g.addPass("c2", &c2);
    g.addPass("add", &add);
    g.addPass("out", &out);
    g.connect("c1.out", "add.a");
    g.connect("c2.out", "add.b");
    g.connect("add.sum", "out.in");

    auto graph = g.compile();
    CHECK(graph);
    if (!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(out.result, 5);
}

// 한 번 만든 그래프를 매 프레임 다시 실행한다.
void executeAgain() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    Capture out;
    g.addPass("c", &c);
    g.addPass("a", &a);
    g.addPass("out", &out);
    g.connect("c.out", "a.in");
    g.connect("a.out", "out.in");

    auto graph = g.compile();
    CHECK(graph);
    if (!graph) {
        return;
    }
    c.value = 1;
    graph->execute();
    CHECK_EQ(out.result, 2);
    c.value = 10;
    graph->execute();
    CHECK_EQ(out.result, 11);
}

void cycleFails() {
    RD::GraphBuilder g;
    AddOne x;
    AddOne y;
    g.addPass("x", &x);
    g.addPass("y", &y);
    g.connect("x.out", "y.in");
    g.connect("y.out", "x.in");
    CHECK(!g.compile());
}

// 연결 안 된 입력이 하나라도 있으면 실패한다(graph-builder.cc의 TODO를 구현하면 통과).
void missingInputFails() {
    RD::GraphBuilder g;
    Constant c;
    AddOne a;
    g.addPass("c", &c);
    g.addPass("a", &a);  // a.in이 연결되지 않음
    CHECK(!g.compile());
}

// 출력을 아무도 안 읽는 노드는 경고만 하고 compile은 성공한다.
void unusedOutputIsAllowed() {
    RD::GraphBuilder g;
    Constant c;
    g.addPass("c", &c);
    CHECK(g.compile());
}

// 입력 하나에 생산자가 둘이면 connect나 compile 중 한 곳에서 실패해야 한다.
void multipleProducersFail() {
    RD::GraphBuilder g;
    Constant c1;
    Constant c2;
    AddOne a;
    Capture out;
    g.addPass("c1", &c1);
    g.addPass("c2", &c2);
    g.addPass("a", &a);
    g.addPass("out", &out);
    g.connect("a.out", "out.in");
    bool built = g.connect("c1.out", "a.in") && g.connect("c2.out", "a.in") && g.compile();
    CHECK(!built);
}

// 그래프 입력은 바깥 데이터를 bind해서 쓰고, 그래프 출력은 이름으로 읽는다.
void graphInputAndOutput() {
    RD::GraphBuilder g;
    AddOne a;
    g.addPass("a", &a);
    CHECK(g.addInput<int>("seed"));
    CHECK(g.connectInput("seed", "a.in"));
    CHECK(g.exportOutput("result", "a.out"));

    auto graph = g.compile();
    CHECK(graph);
    if(!graph) {
        return;
    }
    CHECK(!graph->inputsBound());
    int seed = 41;
    CHECK(graph->bindInput("seed", &seed));
    CHECK(graph->inputsBound());
    graph->execute();
    CHECK_EQ(*graph->output<int>("result"), 42);
}

void graphInputTypeMismatchFails() {
    RD::GraphBuilder g;
    AddOne a;
    g.addPass("a", &a);
    g.addInput<float>("f");
    CHECK(!g.connectInput("f", "a.in"));
    CHECK(!g.connectInput("missing", "a.in"));
}

// GPU 없이 prepare → record 흐름 확인: prepare에서 만든 핸들이 다음 Pass의 record로 전달된다.
struct FakeContext : public RD::PassContext {
    int created = 0;
    int cleared = 0;
    RD::Texture lastCleared;
    uint32_t width() const override { return 4; }
    uint32_t height() const override { return 4; }
    RD::Texture createTexture(RD::Format, uint32_t, uint32_t) override {
        ++created;
        return RD::Texture{reinterpret_cast<RD::GpuImage *>(static_cast<uintptr_t>(created))};
    }
    void clear(RD::Texture target, float, float, float, float) override {
        ++cleared;
        lastCleared = target;
    }
    void blit(RD::Texture, RD::Texture) override {}
    void blitTo(RD::Texture, RD::Texture, RD::Rect) override {}
};

struct MakeTexture : public RD::Pass {
    DECL_FIELD_OUT(RD::Texture, color);
    void prepare(RD::PassContext &ctx) override { *color = ctx.createTexture(RD::Format::RGBA8, 4, 4); }
};

struct ClearTexture : public RD::Pass {
    DECL_FIELD_IN(RD::Texture, color);
    void record(RD::PassContext &ctx) override { ctx.clear(*color, 0, 0, 0, 1); }
};

void prepareThenRecord() {
    RD::GraphBuilder g;
    MakeTexture make;
    ClearTexture clear;
    g.addPass("clear", &clear);
    g.addPass("make", &make);
    g.connect("make.color", "clear.color");

    auto graph = g.compile();
    CHECK(graph);
    if(!graph) {
        return;
    }
    FakeContext ctx;
    graph->prepare(ctx);
    graph->record(ctx);
    graph->record(ctx);
    CHECK_EQ(ctx.created, 1);
    CHECK_EQ(ctx.cleared, 2);
    CHECK(ctx.lastCleared.image == reinterpret_cast<RD::GpuImage *>(uintptr_t(1)));
}

} // namespace

int main() {
    RUN(chain);
    RUN(addOrderDoesNotMatter);
    RUN(fanOut);
    RUN(join);
    RUN(executeAgain);
    RUN(cycleFails);
    RUN(missingInputFails);
    RUN(unusedOutputIsAllowed);
    RUN(multipleProducersFail);
    RUN(graphInputAndOutput);
    RUN(graphInputTypeMismatchFails);
    RUN(prepareThenRecord);
    return TEST_RESULT();
}
