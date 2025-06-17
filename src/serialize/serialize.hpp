#pragma once

#include "util/assert.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/optional.hpp"
#include "util/collections.hpp"
#include "util/allocator.hpp"
#include "util/alloc_ptr.hpp"

#include "serialize/serializer.hpp"
#include "serialize/context.hpp"
#include "serialize/annotations.hpp"
#include "serialize/archive.hpp"
#include "serialize/math.hpp"
#include "serialize/enum.hpp"
#include "serialize/string.hpp"
#include "serialize/type_id.hpp"
#include "serialize/vector.hpp"
#include "serialize/optional.hpp"
#include "serialize/record.hpp"
#include "serialize/array.hpp"

// internal use only
#define DECL_SERIALIZER_IMPL(...)                        \
    static const auto UNUSED CONCAT(_ser, __COUNTER__) = \
        (&Serializer<__VA_ARGS__>::serialize,            \
         &Serializer<__VA_ARGS__>::deserialize);

// declare serializer for T WITHOUT inducing reflection on T
#define DECL_PRIMITIVE_SERIALIZER(...)                   \
    DECL_SERIALIZER_IMPL(__VA_ARGS__)                    \
    ARCHIMEDES_REFLECT_TYPE(                             \
        Serializer<__VA_ARGS__>)                         \
    ARCHIMEDES_REFLECT_TYPE(                             \
        SerializerImpl<__VA_ARGS__>)

// declare a primitive/optional serializer
#define DECL_PRIMITIVE_SERIALIZER_OPT(...)               \
    DECL_PRIMITIVE_SERIALIZER(__VA_ARGS__)               \
    DECL_PRIMITIVE_SERIALIZER(std::optional<__VA_ARGS__>)

// declare regular serializer
#define DECL_SERIALIZER(...)                             \
    ARCHIMEDES_REFLECT_TYPE(__VA_ARGS__)                 \
    DECL_SERIALIZER_IMPL(__VA_ARGS__)                    \
    DECL_SERIALIZER_IMPL(__VA_ARGS__*)                   \
    ARCHIMEDES_REFLECT_TYPE(                             \
        Serializer<__VA_ARGS__>)                         \
    ARCHIMEDES_REFLECT_TYPE(                             \
        SerializerImpl<__VA_ARGS__>)                     \
    ARCHIMEDES_REFLECT_TYPE(                             \
        Serializer<__VA_ARGS__*>)                        \
    ARCHIMEDES_REFLECT_TYPE(                             \
        SerializerImpl<__VA_ARGS__*>)

// declare optional serializer
#define DECL_SERIALIZER_OPT(...)                         \
    DECL_SERIALIZER(__VA_ARGS__)                         \
    DECL_PRIMITIVE_SERIALIZER(std::optional<__VA_ARGS__>)
