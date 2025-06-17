#pragma once

#include "serialize/serializer.hpp"
#include "serialize/archive.hpp"
#include "serialize/annotations.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"

// serializer for numeric types
#define DECL_NUMERIC_SERIALIZER(_T)                                            \
    template <>                                                                \
    struct [[SERIALIZER]] Serializer<_T> : SerializerImpl<_T> {                \
        void serialize(                                                        \
            const _T &data,                                                    \
            Archive &archive,                                                  \
            SerializationContext&) const {                                     \
            SERIALIZE_LOG("{} {}", arc::type_name<_T>(), data);                \
            archive.write(                                                     \
                std::span { reinterpret_cast<const u8*>(&data), sizeof(_T) }); \
        }                                                                      \
                                                                               \
        void deserialize(                                                      \
            _T &data,                                                          \
            Archive &archive,                                                  \
            SerializationContext&) const {                                     \
            archive.read(                                                      \
                std::span { reinterpret_cast<u8*>(&data), sizeof(_T) });       \
            SERIALIZE_LOG("{} {}", arc::type_name<_T>(), data);                \
        }                                                                      \
    };

DECL_NUMERIC_SERIALIZER(char)
DECL_NUMERIC_SERIALIZER(bool)

DECL_NUMERIC_SERIALIZER(u8)
DECL_NUMERIC_SERIALIZER(u16)
DECL_NUMERIC_SERIALIZER(u32)
DECL_NUMERIC_SERIALIZER(u64)
DECL_NUMERIC_SERIALIZER(u128)
DECL_NUMERIC_SERIALIZER(usize)

DECL_NUMERIC_SERIALIZER(i8)
DECL_NUMERIC_SERIALIZER(i16)
DECL_NUMERIC_SERIALIZER(i32)
DECL_NUMERIC_SERIALIZER(i64)
DECL_NUMERIC_SERIALIZER(i128)
DECL_NUMERIC_SERIALIZER(isize)

DECL_NUMERIC_SERIALIZER(f32)
DECL_NUMERIC_SERIALIZER(f64)

// vector type serializer
template <math::is_vec_c T>
struct [[SERIALIZER]] Serializer<T>
    : Serializer<typename math::vec_traits<T>::T>,
      SerializerImpl<T> {
    using traits = math::vec_traits<T>;
    using Base = Serializer<typename traits::T>;

    void serialize(
        const T &data,
        Archive &archive,
        SerializationContext &ctx) const {
        for (size_t i = 0; i < traits::L; i++) {
            Base::serialize(data[i], archive, ctx);
        }
    }

    void deserialize(
        T &data,
        Archive &archive,
        SerializationContext &ctx) const {
        for (size_t i = 0; i < traits::L; i++) {
            Base::deserialize(data[i], archive, ctx);
        }
    }
};
