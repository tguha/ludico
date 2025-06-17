#pragma once

#include "serialize/annotations.hpp"
#include "serialize/serializer.hpp"
#include "serialize/math.hpp"

// string serializer
template <>
struct [[SERIALIZER]] Serializer<std::string>
    : Serializer<char>,
      SerializerImpl<
        std::string,
        Serializer<std::string>> {

    constexpr static arc::type_id override_type_id() {
        return arc::type_id::from("std::basic_string<char>");
    }

    void serialize(
        const std::string &data,
        Archive &archive,
        SerializationContext &ctx) const {
        Serializer<size_t>().serialize(data.length(), archive, ctx);
        for (const auto &c : data) {
            Serializer<char>::serialize(c, archive, ctx);
        }
    }

    void deserialize(
        std::string &data,
        Archive &archive,
        SerializationContext &ctx) const {
        size_t sz;
        Serializer<size_t>().deserialize(sz, archive, ctx);
        data.resize(sz);
        for (size_t i = 0; i < sz; i++) {
            Serializer<char>::deserialize(data[i], archive, ctx);
        }
    }
};
