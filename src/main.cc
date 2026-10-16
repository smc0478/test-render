/*
 * 프로그램 진입점 main의 실행 흐름 초안을 담는다.
 * 기본 모듈 등록, 플러그인 등록, 그래프 구성 순서와 실패 시 종료 처리를 포함한다.
 * 실행 루프는 비어 있으며, 호출 대상의 연결과 실제 실행 처리는 아직 구현되지 않았다.
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