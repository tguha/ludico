#pragma once

#include <archimedes/type_id.hpp>
namespace arc = archimedes;

#include "util/types.hpp"
#include "util/util.hpp"
#include "serialize/annotations.hpp"

// serializers should be annotated with SERIALIZER
#define SERIALIZER_TEXT "_serializer_"
#define SERIALIZER ARCHIMEDES_ANNOTATE(SERIALIZER_TEXT)

struct Archive;
struct SerializationContext;

// all serializers must match concept
template <
    typename SerializerType,
    typename T = typename SerializerType::serialize_type>
concept IsSerializer =
    requires (
        const SerializerType &s,
        Archive &a,
        SerializationContext &ctx,
        const T &t_const,
        T &t) {
        { s.serialize(t_const, a, ctx) } -> std::same_as<void>;
        { s.deserialize(t, a, ctx) } -> std::same_as<void>;
    };

// base serializer which is specialized for specific types
template <typename T>
struct [[SERIALIZER]] Serializer;

// all serializers must inherit from SerializerImpl
template <typename T, typename S = void>
struct SerializerImpl {
    // type which this serializer serializers
    using serialize_type = T;

    // ID which this SerializerImpl serializes
    static constexpr arc::type_id serialize_type_id =
        []() {
            if constexpr (!std::is_same_v<S, void>) {
                return S::override_type_id();
            }

            return arc::type_id::from<T>();
        }();
};
