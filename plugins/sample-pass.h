/*
 * 사용자 정의 스키마와 Pass의 사용 문법을 검토하는 예시 초안을 담는다.
 * 비어 있는 SampleSchema, int 입력·출력을 선언하는 SamplePass,
 * 입력 역참조·멤버 접근 및 connect 연결 예시를 포함한다.
 * 현재 접근·연결 예시는 의사 코드이며, 완성된 C++ 구현은 아니다.
 */

#include "graph-ir.h"
#include "schema.h"

class SampleSchema: RD::SchemaDesc {

private:
};

class SamplePass: RD::GraphIr {
*in 
in->a
private:
    DECL_FIELD_IN(int, in);
    DECL_FIELD_OUT(int, out);
};

connect("SamplePass.out", "SamplePass.in")