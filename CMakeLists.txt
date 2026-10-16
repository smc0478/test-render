# 파일럿 렌더러 빌드.
# 지금은 1단계(GPU 없이 도는 Pass·그래프 코어)와 그 테스트만 빌드한다.
# 앱(src/main.cc 등)과 GPU 쪽 파일은 해당 단계를 구현할 때 여기에 추가한다.
cmake_minimum_required(VERSION 3.20)
project(render LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # 소스가 BOM 없는 UTF-8(한글 주석)이라 MSVC가 CP949로 잘못 읽지 않게 한다.
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 1단계 코어: 기초 타입, Pass·필드, GraphBuilder(최소 컴파일 포함).
add_library(rd-graph STATIC
    src/core.cc
    src/pass.cc
    src/graph-builder.cc
)
target_include_directories(rd-graph PUBLIC include)

enable_testing()
add_subdirectory(tests)
