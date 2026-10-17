# 파일럿 렌더러 빌드.
# - rd-graph: GPU 없이 도는 Pass·스키마·그래프 코어(단위 테스트 대상).
# - rd-engine: Vulkan 런타임, 장면 로딩, 내장 Pass, 플러그인 로더.
# - RendererPilot: 앱. plugins/의 EdgeMask / InkOverlay / SponzaStylizedGraph는 독립 DLL.
# 셰이더(Slang)는 아직 없다. GPU Pass는 clear / blit으로만 동작한다.
cmake_minimum_required(VERSION 3.24)
project(render LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RD_BUILD_GPU "Vulkan 런타임, 앱, 플러그인까지 빌드" ON)

if(MSVC)
    # 소스가 BOM 없는 UTF-8(한글 주석)이라 MSVC가 CP949로 잘못 읽지 않게 한다.
    set(RD_WARNINGS /utf-8 /W4)
else()
    set(RD_WARNINGS -Wall -Wextra)
endif()

# MinGW: 실행 파일 옆에 컴파일러와 같은 버전의 런타임 DLL을 둔다. PATH에 다른 버전
# (예: MSYS2 /ucrt64/bin)이 먼저 있으면 실행 중 진입점을 못 찾아 죽기 때문이다.
function(rd_copy_runtime target)
    if(MINGW)
        get_filename_component(compiler_bin ${CMAKE_CXX_COMPILER} DIRECTORY)
        foreach(dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
            if(EXISTS ${compiler_bin}/${dll})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${compiler_bin}/${dll} $<TARGET_FILE_DIR:${target}>)
            endif()
        endforeach()
    endif()
endfunction()

# 1단계 코어: 기초 타입, 스키마, Pass·필드, 등록부, GraphBuilder.
add_library(rd-graph STATIC
    src/core.cc
    src/schema.cc
    src/pass.cc
    src/pass-registry.cc
    src/graph-builder.cc
)
target_include_directories(rd-graph PUBLIC include)
target_compile_options(rd-graph PRIVATE ${RD_WARNINGS})

enable_testing()

if(RD_BUILD_GPU)
    include(FetchContent)

    FetchContent_Declare(vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v1.3.295
        GIT_SHALLOW TRUE
        SYSTEM)
    # volk은 자체 CMake 대신 volk.c만 직접 빌드한다(SOURCE_SUBDIR로 add_subdirectory를 막음).
    FetchContent_Declare(volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG 1.3.295
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR rd-no-cmake)
    # fastgltf가 쓰는 simdjson. fastgltf의 자체 다운로드(file(DOWNLOAD))가 이 환경에서 빈 파일을 받아서,
    # git으로 받은 단일 헤더 버전을 simdjson::simdjson 타깃으로 미리 만들어 둔다.
    FetchContent_Declare(simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v3.9.4
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR rd-no-cmake)
    FetchContent_Declare(fastgltf
        GIT_REPOSITORY https://github.com/spnda/fastgltf.git
        GIT_TAG v0.8.0
        GIT_SHALLOW TRUE
        SYSTEM)
    # C 헤더만 쓴다. C++ 모듈(vulkan.cppm)은 MinGW GCC에서 빌드되지 않아 끈다.
    set(VULKAN_HEADERS_ENABLE_MODULE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(vulkan_headers volk simdjson)

    add_library(rd-simdjson STATIC ${simdjson_SOURCE_DIR}/singleheader/simdjson.cpp)
    target_include_directories(rd-simdjson PUBLIC $<BUILD_INTERFACE:${simdjson_SOURCE_DIR}/singleheader>)
    if(MINGW)
        # MinGW GCC는 스택을 16바이트까지만 정렬해 AVX2(haswell)·AVX-512(icelake) 경로가 크래시한다
        # (GCC bug 54412). SSE4.2(westmere) 이하만 쓴다.
        target_compile_definitions(rd-simdjson PUBLIC SIMDJSON_AVX512_ALLOWED=0
                                                      SIMDJSON_IMPLEMENTATION_ICELAKE=0
                                                      SIMDJSON_IMPLEMENTATION_HASWELL=0)
    endif()
    add_library(simdjson::simdjson ALIAS rd-simdjson)

    FetchContent_MakeAvailable(fastgltf)
    # fastgltf의 install(EXPORT)가 의존 타깃도 export 묶음에 있기를 요구한다(설치는 하지 않음).
    install(TARGETS rd-simdjson EXPORT fastgltf-targets ARCHIVE DESTINATION lib)

    # Vulkan SDK 없이 vulkan-1.dll을 실행 중에 불러오는 로더.
    add_library(rd-volk STATIC ${volk_SOURCE_DIR}/volk.c)
    target_include_directories(rd-volk PUBLIC ${volk_SOURCE_DIR})
    target_link_libraries(rd-volk PUBLIC Vulkan::Headers)
    target_compile_definitions(rd-volk PUBLIC VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR
                                              WIN32_LEAN_AND_MEAN NOMINMAX)

    add_library(rd-engine STATIC
        src/vulkan.cc
        src/runtime.cc
        src/scene.cc
        src/core-passes.cc
        src/plugin-manager.cc
        src/startup.cc
    )
    target_link_libraries(rd-engine PUBLIC rd-graph rd-volk fastgltf::fastgltf)
    target_compile_options(rd-engine PRIVATE ${RD_WARNINGS})
    if(NOT MSVC)
        # Vk 구조체를 {sType}만 주고 나머지를 0으로 두는 관용구 때문에 끈다.
        target_compile_options(rd-engine PRIVATE -Wno-missing-field-initializers)
    endif()

    add_executable(RendererPilot src/main.cc src/app.cc)
    target_link_libraries(RendererPilot PRIVATE rd-engine)
    rd_copy_runtime(RendererPilot)
    target_compile_options(RendererPilot PRIVATE ${RD_WARNINGS})
    target_compile_definitions(RendererPilot PRIVATE
        RD_DEFAULT_ASSET="${CMAKE_SOURCE_DIR}/sponza-gltf-main/sponza-png.glb"
        RD_DEFAULT_PLUGIN_DIR="${CMAKE_BINARY_DIR}/plugins")

    # 플러그인 DLL: 호스트 소스 없이 SDK 헤더(include/)와 rd-graph만 쓴다.
    function(rd_add_plugin name source)
        add_library(${name} SHARED ${source})
        target_link_libraries(${name} PRIVATE rd-graph)
        target_include_directories(${name} PRIVATE plugins)
        target_compile_options(${name} PRIVATE ${RD_WARNINGS})
        set_target_properties(${name} PROPERTIES
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
        add_dependencies(RendererPilot ${name})
    endfunction()

    rd_add_plugin(EdgeMask plugins/edge-mask.cc)
    rd_add_plugin(InkOverlay plugins/ink-overlay.cc)
    rd_add_plugin(SponzaStylizedGraph plugins/stylized-graph.cc)
endif()

add_subdirectory(tests)
