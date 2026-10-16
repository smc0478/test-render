/*
 * Pass 사이에서 전달할 데이터 참조의 기본 틀을 정의하는 파일.
 * - RData<T>: 타입 T의 데이터 포인터를 보관할 템플릿 초안.
 * - 데이터 접근, 생성 및 수명 관리 기능은 아직 구현하지 않았다.
 */
#ifndef _DATA_H_
#define _DATA_H_

namespace RD {

template<typename T>
class RData<T> {

private:
    T *data;
};

}

#endif // data.h