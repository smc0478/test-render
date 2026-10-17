/*
 * 모든 파일이 쓰는 기초 타입. 표준 라이브러리만 써서 GPU·DLL 없이 테스트할 수 있게 한다.
 * - Severity, Diagnostic: 그래프 조립·컴파일의 오류와 경고. 실패한 후보 그래프는 이것만 남기고 버린다.
 * - 필요: 그래프가 커지면 StrongId, Version(QualifiedName@버전), Result<T>를 추가.
 * - 범위: SDK 공개. 구현: src/core.cc
 */
#pragma once

#include <string>
#include <vector>

namespace RD {

enum class Severity {
    Warning,
    Error
};

struct Diagnostic {
    Severity severity;
    std::string message;
};

// "[error] ..." / "[warning] ..."
std::string toString(const Diagnostic &diagnostic);

}
