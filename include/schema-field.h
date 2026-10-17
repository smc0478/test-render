/*
 * 타입별 스키마 선언. 기존 C++ 구조체에 DEFINE_STRUCT_SCHEM(Type, (fields...)) 한 줄로 노출 필드만 지정한다.
 * - SchemaTraits<T>: T의 스키마. 선언하지 않은 타입은 노출 필드 없는 opaque 스키마(크기·정렬만).
 * - 기본형: bool, int32, uint32, float32.
 * - DEFINE_STRUCT_SCHEM(Type, (a, b)): 노출 필드 a, b와 실제 offset. 노출 필드는 최대 16개.
 *   키를 직접 정하려면 DEFINE_NAMED_STRUCT_SCHEM(Type, "core.Albedo", (tex)).
 * - DEFINE_OPAQUE_SCHEM(Type, "key"): 노출 필드 없이 이름만 붙인다(핸들 타입 등).
 * - 두 매크로는 전역 네임스페이스에서 쓴다. 실제 등록은 그래프가 포트를 모을 때 한다.
 * - 범위: SDK 공개.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "schema.h"

namespace RD {

template <typename T>
struct SchemaTraits {
    static const SchemaDesc &desc() {
        static const SchemaDesc d(std::string("opaque:") + typeid(T).name(), sizeof(T), alignof(T));
        return d;
    }
};

#define RD_BASIC_SCHEM(Type, Key)                                    \
    template <>                                                      \
    struct SchemaTraits<Type> {                                      \
        static const SchemaDesc &desc() {                            \
            static const SchemaDesc d(Key, sizeof(Type), alignof(Type)); \
            return d;                                                \
        }                                                            \
    };

RD_BASIC_SCHEM(bool, "bool")
RD_BASIC_SCHEM(int32_t, "int32")
RD_BASIC_SCHEM(uint32_t, "uint32")
RD_BASIC_SCHEM(float, "float32")

#undef RD_BASIC_SCHEM

}

// --- DEFINE_STRUCT_SCHEM 구현용 매크로 ---
#define RD_SCHEM_EXPAND(x) x
#define RD_SCHEM_UNPAREN(...) __VA_ARGS__
#define RD_SCHEM_FIELD(T, f) \
    ::RD::SchemaFieldDesc{#f, &::RD::SchemaTraits<decltype(T::f)>::desc(), offsetof(T, f)}

#define RD_SCHEM_FE_1(M, T, a) M(T, a)
#define RD_SCHEM_FE_2(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_1(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_3(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_2(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_4(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_3(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_5(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_4(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_6(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_5(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_7(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_6(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_8(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_7(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_9(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_8(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_10(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_9(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_11(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_10(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_12(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_11(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_13(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_12(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_14(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_13(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_15(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_14(M, T, __VA_ARGS__))
#define RD_SCHEM_FE_16(M, T, a, ...) M(T, a), RD_SCHEM_EXPAND(RD_SCHEM_FE_15(M, T, __VA_ARGS__))
#define RD_SCHEM_PICK(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define RD_SCHEM_FOR_EACH(M, T, ...)                                                        \
    RD_SCHEM_EXPAND(RD_SCHEM_PICK(__VA_ARGS__, RD_SCHEM_FE_16, RD_SCHEM_FE_15, RD_SCHEM_FE_14, \
                                  RD_SCHEM_FE_13, RD_SCHEM_FE_12, RD_SCHEM_FE_11, RD_SCHEM_FE_10, \
                                  RD_SCHEM_FE_9, RD_SCHEM_FE_8, RD_SCHEM_FE_7, RD_SCHEM_FE_6,     \
                                  RD_SCHEM_FE_5, RD_SCHEM_FE_4, RD_SCHEM_FE_3, RD_SCHEM_FE_2,     \
                                  RD_SCHEM_FE_1)(M, T, __VA_ARGS__))

#define DEFINE_NAMED_STRUCT_SCHEM(Type, Key, fields)                                        \
    template <>                                                                             \
    struct RD::SchemaTraits<Type> {                                                         \
        static_assert(std::is_standard_layout_v<Type>, #Type " must be standard-layout");   \
        static const ::RD::SchemaDesc &desc() {                                             \
            static const ::RD::SchemaDesc d(                                                \
                Key, sizeof(Type), alignof(Type),                                           \
                {RD_SCHEM_FOR_EACH(RD_SCHEM_FIELD, Type, RD_SCHEM_UNPAREN fields)});        \
            return d;                                                                       \
        }                                                                                   \
    };

#define DEFINE_STRUCT_SCHEM(Type, fields) DEFINE_NAMED_STRUCT_SCHEM(Type, #Type, fields)

#define DEFINE_OPAQUE_SCHEM(Type, Key)                                                      \
    template <>                                                                             \
    struct RD::SchemaTraits<Type> {                                                         \
        static const ::RD::SchemaDesc &desc() {                                             \
            static const ::RD::SchemaDesc d(Key, sizeof(Type), alignof(Type));              \
            return d;                                                                       \
        }                                                                                   \
    };
