/*
 * 앱 시작 단계: 플러그인 등록과 루트 그래프 구성. main()이 순서대로 부른다. 선언은 app.h.
 * (원래 graph-build.cc — GraphBuilder 구현이 아니라 시작 절차라서 변경. GraphBuilder 구현은 graph-builder.cc)
 * - 있는 것: RegistryPlugins(비어 있음), BuildGraph(GraphBuilder 생성·AddPass 예시).
 * - 필요: include(app.h, graph-builder.h), plugin-manager로 DLL 로드, 구성 → compile → 런타임 전달,
 *   bool 반환(지금 return 없음, AddPass 인자·세미콜론 누락).
 */


bool RegistryPlugins() {

}

bool BuildGraph() {
    GraphBuilder g;

    g.AddPass(name)
}