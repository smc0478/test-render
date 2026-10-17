# 테스트. 항목마다 실행 파일을 따로 둬서, 구현 중인 항목만 빌드·실행할 수 있다.
# GPU 없이: ctest --test-dir build -LE gpu
#   cmake -S . -B build -G Ninja
#   cmake --build build --target test-pass-field
#   ctest --test-dir build -R pass-field --output-on-failure
# 전부: cmake --build build && ctest --test-dir build --output-on-failure
function(rd_add_test name)
    add_executable(${name} ${name}.cc)
    target_link_libraries(${name} PRIVATE rd-graph)
    rd_copy_runtime(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

rd_add_test(test-pass-field)     # 1번: 필드 등록
rd_add_test(test-pass-data)      # 2번: bind로 데이터 접근
rd_add_test(test-graph-builder)  # 3번: addPass, connect
rd_add_test(test-graph-run)      # 4번: compile, CPU 실행, 그래프 입출력, prepare → record
rd_add_test(test-schema)         # 5번: 스키마, 타입 검사, 하위 필드 연결
rd_add_test(test-registry)       # 6·7번: 등록부, GraphPass 전개

# GPU 쪽(라이브러리는 쓰지만 GPU 장치는 만들지 않음) + 앱 실행 확인.
if(RD_BUILD_GPU)
    add_executable(test-asset test-asset.cc)
    target_link_libraries(test-asset PRIVATE rd-engine)
    target_compile_definitions(test-asset PRIVATE RD_ASSET_DIR="${CMAKE_SOURCE_DIR}/sponza-gltf-main")
    rd_copy_runtime(test-asset)
    add_test(NAME test-asset COMMAND test-asset)

    add_executable(test-plugin test-plugin.cc)
    target_link_libraries(test-plugin PRIVATE rd-engine)
    target_compile_definitions(test-plugin PRIVATE RD_PLUGIN_DIR="${CMAKE_BINARY_DIR}/plugins")
    add_dependencies(test-plugin EdgeMask InkOverlay SponzaStylizedGraph)
    rd_copy_runtime(test-plugin)
    add_test(NAME test-plugin COMMAND test-plugin)

    # 창을 띄워 실제 GPU로 120프레임 그리고, 30프레임마다 outline을 토글해 그래프 교체까지 확인한다.
    add_test(NAME app-smoke COMMAND RendererPilot --frames 120 --toggle-every 30)
    set_tests_properties(app-smoke PROPERTIES LABELS gpu)
endif()
