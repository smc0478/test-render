/*
 * 프로그램 진입점. 시작 순서(기본 모듈 → 플러그인 → 그래프)를 한눈에 보이게 두고, 실패하면 렌더링 전에 끝낸다.
 * - 있는 것: RegistryDefaultModule / RegistryPlugins / BuildGraph 호출, 빈 실행 루프.
 * - 필요: include(core-passes.h, app.h), 루프에서 창 이벤트 → 설정 변경 시 재구성 → 프레임 실행,
 *   종료 조건.
 */



int main(int argc, char **argv) {


    RegistryDefaultModule();

    if(!RegistryPlugins()) {
        return -1;
    }

    if(!BuildGraph()) {
        return -1;
    }

    while(true) {

    }
}