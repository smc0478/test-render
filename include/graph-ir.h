/*
 * 그래프에서 사용하는 Pass 공통 인터페이스의 초안을 정의하는 파일.
 * - FieldBase 전방 선언: 입출력 필드를 포인터로 참조하기 위한 선언.
 * - GraphIr: execute 실행 인터페이스, 식별자, 이름별 입출력 필드 맵을 보관.
 * - registerInputField, registerOutputField: 필드 등록 함수의 선언.
 * - 필드 등록 처리와 구체적인 Pass 실행 동작은 아직 구현하지 않았다.
 */
#pragma once

#include <unordered_map>
#include <string>

namespace RD {

class FieldBase;  // Forward Declaration

class GraphIr {
public:

    virtual void execute() = 0;
private:
    void registerInputField();
    void registerOutputField();
    int id;
    std::unordered_map<std::string, FieldBase*> input;
    std::unordered_map<std::string, FieldBase*> output; 
};

}
