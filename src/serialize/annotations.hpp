#pragma once

#include <archimedes/basic.hpp>

#include "util/macros.hpp"
#include "util/log.hpp"

// uncomment to enable verbose logging for serialiation
// #define SERIALIZE_VERBOSE

#ifdef SERIALIZE_VERBOSE
#define SERIALIZE_LOG(...) LOG(__VA_ARGS__)
#else
#define SERIALIZE_LOG(...)
#endif

// types must be marked with SERIALIZABLE to be serialized
#define SERIALIZABLE ARCHIMEDES_REFLECT

// mark translation unit as serializable
#define SERIALIZE_ENABLE() ARCHIMEDES_ARG("enable")

// fields can be marked with [[SERIALIZE_IGNORE]] to avoid serialization
#define SERIALIZE_IGNORE_TEXT "serialize_ignore"
#define SERIALIZE_IGNORE ARCHIMEDES_ANNOTATE(SERIALIZE_IGNORE_TEXT)

// TODO: doc
#define SERIALIZABLE_TYPE(...)                                                 \
    ARCHIMEDES_REFLECT_TYPE(__VA_ARGS__)

// TODO: doc
#define SERIALIZABLE_TYPE_REGEX(...)                                           \
    ARCHIMEDES_REFLECT_TYPE_REGEX(__VA_ARGS__)

// TODO: doc
#define SERIALIZABLE_TYPE_REGEX(...)                                           \
    ARCHIMEDES_REFLECT_TYPE_REGEX(__VA_ARGS__)

// TODO: doc
#define SERIALIZE_POLY                                                         \
    ARCHIMEDES_ANNOTATE("serialize_poly")
#define SERIALIZE_POLY_TEXT "serialize_poly"

// use to denote a type as serializable but with a different kind of serializer
// fx.
//
// struct Bar { int x; };
// struct Foo { /* ... */ } SERIALIZE_AS(Bar);
//
// Foo is serialized/deserialized as though it were a Bar
#define SERIALIZE_AS(...)                                                      \
    ARCHIMEDES_ANNOTATE("serialize_as" CONCAT_ARGS_N(__VA_ARGS__))
#define SERIALIZE_AS_TEXT "serialize_as"

// TODO: doc
#define SERIALIZE_BY_ID(...)                                                   \
    ARCHIMEDES_ANNOTATE("serialize_by_id_" #__VA_ARGS__)
#define SERIALIZE_BY_ID_TEXT "serialize_by_id_"

// TODO: doc
#define SERIALIZE_BY_CTX(...)                                                  \
    ARCHIMEDES_ANNOTATE("serialize_by_ctx_" #__VA_ARGS__)
#define SERIALIZE_BY_CTX_TEXT "serialize_by_ctx_"

// TODO: doc
#define SERIALIZE_CUSTOM(...)                                                  \
    ARCHIMEDES_ANNOTATE("serialize_custom_" #__VA_ARGS__)
#define SERIALIZE_CUSTOM_TEXT "serialize_custom_"

struct SerializationContext;
