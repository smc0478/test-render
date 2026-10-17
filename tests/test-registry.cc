/*
 * 2단계 6·7번 테스트: 등록부로 Pass 만들기(pass-registry.h)와 GraphPass 전개.
 * - 타입 이름으로 생성, 모르는 타입 실패, 만든 Pass는 빌더가 사라져도 Graph가 소유,
 *   GraphPass가 설정(extra)에 따라 다른 그래프를 만든다.
 */
#include "graph-builder.h"
#include "pass-registry.h"
#include "test-passes.h"
#include "test.h"

namespace {

// c → a → (extra면 a2 →) 결과
struct ChainGraph : public RD::GraphPass {
    bool expand(RD::GraphBuilder &g, const RD::GraphBuildContext &ctx) override {
        bool extra = ctx.getBool("extra", false);
        if(!g.createPass("test.Constant", "c") || !g.createPass("test.AddOne", "a"))
            return false;
        if(!g.connect("c.out", "a.in"))
            return false;
        if(!extra)
            return g.exportOutput("result", "a.out");

        if(!g.createPass("test.AddOne", "a2") || !g.connect("a.out", "a2.in"))
            return false;
        return g.exportOutput("result", "a2.out");
    }
};

RD::PassRegistry makeRegistry() {
    RD::PassRegistry registry;
    registry.add<Constant>("test.Constant");
    registry.add<AddOne>("test.AddOne");
    registry.add<Capture>("test.Capture");
    registry.addGraph<ChainGraph>("test.Chain");
    return registry;
}

void createByType() {
    RD::PassRegistry registry = makeRegistry();
    RD::GraphBuilder g(&registry);
    CHECK(g.createPass("test.AddOne", "a") != nullptr);
    CHECK(g.createPass("test.Nope", "x") == nullptr);
    CHECK(!registry.add<AddOne>("test.AddOne"));  // 같은 타입 두 번 등록 불가
}

void createdPassesLiveWithGraph() {
    RD::PassRegistry registry = makeRegistry();
    std::unique_ptr<RD::Graph> graph;
    {
        RD::GraphBuilder g(&registry);
        g.createPass("test.Constant", "c");
        g.createPass("test.AddOne", "a");
        g.connect("c.out", "a.in");
        g.exportOutput("result", "a.out");
        graph = g.compile();
    }  // 빌더가 사라져도 Pass는 Graph가 갖고 있다.
    CHECK(graph);
    if(!graph) {
        return;
    }
    graph->execute();
    CHECK_EQ(*graph->output<int>("result"), 1);  // Constant 기본값 0 + 1
}

void graphPassExpands() {
    RD::PassRegistry registry = makeRegistry();
    RD::GraphBuildContext ctx(&registry);

    RD::GraphBuilder plain(&registry);
    CHECK(plain.expand("test.Chain", ctx));
    auto g1 = plain.compile();
    CHECK(g1);

    ctx.set("extra", true);
    RD::GraphBuilder extra(&registry);
    CHECK(extra.expand("test.Chain", ctx));
    auto g2 = extra.compile();
    CHECK(g2);
    if(!g1 || !g2) {
        return;
    }
    CHECK_EQ(g1->nodes().size(), size_t(2));
    CHECK_EQ(g2->nodes().size(), size_t(3));
    g1->execute();
    g2->execute();
    CHECK_EQ(*g1->output<int>("result"), 1);
    CHECK_EQ(*g2->output<int>("result"), 2);
}

void contextAndUnknownGraph() {
    RD::PassRegistry registry = makeRegistry();
    RD::GraphBuildContext ctx(&registry);
    CHECK(ctx.hasPass("test.AddOne"));
    CHECK(!ctx.hasPass("test.Nope"));
    CHECK(ctx.getBool("missing", true));

    RD::GraphBuilder g(&registry);
    CHECK(!g.expand("test.Nope", ctx));
}

} // namespace

int main() {
    RUN(createByType);
    RUN(createdPassesLiveWithGraph);
    RUN(graphPassExpands);
    RUN(contextAndUnknownGraph);
    return TEST_RESULT();
}
