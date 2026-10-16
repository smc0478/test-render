/*
 * 테스트용 최소 도구. 외부 라이브러리 없이 실패 위치를 찍고, 실패가 있으면 0이 아닌 값으로 끝낸다.
 * - CHECK(cond), CHECK_EQ(a, b): 실패해도 계속 진행하고 실패 수만 센다.
 * - RUN(fn): 테스트 함수 하나 실행 후 [ OK ] / [FAIL] 출력.
 * - TEST_RESULT(): main의 반환값.
 * - 콘솔 인코딩 문제를 피하려고 실행 중 출력은 영어로 한다.
 */
#pragma once

#include <cstdio>

namespace rd_test {
inline int failures = 0;
}

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
            ++rd_test::failures;                                                 \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                           \
    do {                                                                         \
        auto rd_a_ = (a);                                                        \
        auto rd_b_ = (b);                                                        \
        if (!(rd_a_ == rd_b_)) {                                                 \
            std::printf("  %s:%d: CHECK_EQ(%s, %s) failed\n",                    \
                        __FILE__, __LINE__, #a, #b);                             \
            ++rd_test::failures;                                                 \
        }                                                                        \
    } while (0)

#define RUN(fn)                                                                  \
    do {                                                                         \
        int rd_before_ = rd_test::failures;                                      \
        fn();                                                                    \
        std::printf("%s %s\n",                                                   \
                    rd_test::failures == rd_before_ ? "[ OK ]" : "[FAIL]", #fn);  \
    } while (0)

#define TEST_RESULT() (rd_test::failures == 0 ? 0 : 1)
