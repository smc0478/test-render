# 1단계 테스트. 항목마다 실행 파일을 따로 둬서, 구현 중인 항목만 빌드·실행할 수 있다.
#   cmake -S . -B build -G Ninja
#   cmake --build build --target test-pass-field
#   ctest --test-dir build -R pass-field --output-on-failure
# 전부: cmake --build build && ctest --test-dir build --output-on-failure
function(rd_add_test name)
    add_executable(${name} ${name}.cc)
    target_link_libraries(${name} PRIVATE rd-graph)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

rd_add_test(test-pass-field)     # 1번: 필드 등록
rd_add_test(test-pass-data)      # 2번: bind로 데이터 접근
rd_add_test(test-graph-builder)  # 3번: addPass, connect
rd_add_test(test-graph-run)      # 4번: compile, CPU 실행, 그래프 입출력, prepare → record
rd_add_test(test-schema)         # 5번: 스키마, 타입 검사, 하위 필드 연결
rd_add_test(test-registry)       # 6·7번: 등록부, GraphPass 전개
