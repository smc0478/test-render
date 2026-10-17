# 스키마 구현 메모

현재는 파일 자리와 전방 선언만 준비한다. 매크로, 등록·조회, 필드 접근 및 Branch IR 동작은 아직 구현하지 않는다.

## 최소 파일 구성

| 파일 | 역할 |
| --- | --- |
| `include/schema.h` | 공통 `SchemaDesc`, 노출 필드 정보(`SchemaFieldDesc`), 호환성·경로 조회, 스키마 등록부 |
| `include/schema-field.h` | `SchemaTraits<T>`(기본형·opaque), `DEFINE_STRUCT_SCHEM`, `DEFINE_OPAQUE_SCHEM` |
| `src/schema.cc` | 스키마·필드 등록과 조회 구현 |

기존 `schema.h`의 클래스들은 작성 중인 초안으로 유지한다. 등록부는 현재 `Schema`를 사용하고, 별도 등록부·컴파일러 파일은 만들지 않는다.

## 데이터와 스키마 선언

데이터는 일반 C++ 타입으로 정의한다. 사용자가 스키마 전용 클래스를 작성하도록 강제하지 않는다. 기본형은 SDK가 지원하고, 구조체는 노출할 필드만 지정한다. 다음은 구현할 API 예시다.

```cpp
struct RenderSettings {
    uint32_t quality;
    float internalScale;
    bool shadows;
};

DEFINE_STRUCT_SCHEM(RenderSettings, (quality, shadows));

// Pass 포트는 기존 선언 방식을 유지한다.
DECL_FIELD_IN(RenderSettings, settings);
```

- 스키마는 렌더러의 타입이자 in/out 연결 계약이다. 타입 식별과 필요한 버전 정보를 둔다.
- `size`와 `alignment`는 전체 데이터 타입의 `sizeof(T)`, `alignof(T)`로 얻는다.
- 노출 필드에는 이름, 타입, 실제 offset을 기록한다. 노출한 필드만 재배치하지 않는다.
- offset 기반 등록은 우선 standard-layout 타입의 접근 가능한 멤버를 대상으로 한다.
- 매크로는 타입별 스키마 정의를 제공하고, 실제 등록은 그래프가 포트를 수집할 때 수행한다.
- `settings->quality`는 유효한 `RenderSettings` 인스턴스에 대한 C++ 접근이다. 노출 목록은 Branch IR과 동적 조회의 대상을 정한다.
- 스키마 정의와 데이터 인스턴스를 구분한다. 실제 값의 저장 공간과 수명은 실행 준비·바인딩 단계에서 관리한다.

## 등록과 조회

- 스키마는 키로, 노출 필드는 이름으로 `unordered_map`에서 조회한다. offset은 map 순서와 관계없다.
- 파생 descriptor를 보존하도록 등록부는 포인터 소유 형태로 구현한다. 현재 `map<string, SchemaDesc>` 초안은 구현 시 조정한다.
- 같은 키의 중복 정의는 타입·배치·노출 필드 정보를 확인한다. 그래프가 사용하는 동안 정의를 고정한다.
- 기본 메모리 정보만으로 임의의 객체를 바이트 복사하거나 파괴하지 않는다. 복잡한 값의 수명 처리는 별도로 정한다.

## 이후 Branch IR 작업

- 조건은 입력 데이터의 노출 필드, 상수, 대소·동등 비교 연산으로 표현한다. 비교 가능한 타입인지 검사한다.
- 초기에는 프레임 시작 시 CPU에서 읽을 수 있는 데이터를 조건으로 사용한다.
- 후보 실행 경로를 미리 검증·준비하고, 조건에 따라 사용할 경로를 선택한다. 분기 결과가 합류하면 출력 스키마가 호환돼야 한다.
- GPU 결과를 읽는 조건과 Branch IR 전용 파일은 해당 기능을 구현할 때 추가한다.

## 외부 데이터

외부 C++ 객체의 필드나 ABI 배치를 자동으로 알아내지 않는다. 외부 타입·API를 아는 Pass가 필요한 값을 읽고 내부 스키마의 데이터로 변환한다. 객체를 그대로 전달할 때는 opaque handle로 취급하며, size·alignment는 핸들 자체를 설명한다. 외부 필드 리플렉션이나 범용 FFI 스키마 등록은 초기 범위에 넣지 않는다.
