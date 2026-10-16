/*
 * 그래프 테스트가 같이 쓰는 int Pass들. GPU 없이 연결·실행 순서를 숫자로 확인하려고 둔다.
 * - Constant: value를 out으로 낸다(시작 노드).
 * - AddOne: in + 1 → out.
 * - Add: a + b → sum(입력 두 개가 합류하는 경우).
 * - Capture: in을 result에 저장한다(끝 노드, 테스트가 결과를 읽는 곳).
 */
#pragma once

#include "pass-field.h"

struct Constant : public RD::Pass {
    DECL_FIELD_OUT(int, out);
    int value = 0;
    void execute() override { *out = value; }
};

struct AddOne : public RD::Pass {
    DECL_FIELD_IN(int, in);
    DECL_FIELD_OUT(int, out);
    void execute() override { *out = *in + 1; }
};

struct Add : public RD::Pass {
    DECL_FIELD_IN(int, a);
    DECL_FIELD_IN(int, b);
    DECL_FIELD_OUT(int, sum);
    void execute() override { *sum = *a + *b; }
};

struct Capture : public RD::Pass {
    DECL_FIELD_IN(int, in);
    int result = 0;
    void execute() override { result = *in; }
};
