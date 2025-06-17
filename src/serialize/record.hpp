#pragma once

// TODO: actual data locality for all serializer elements

#include <archimedes.hpp>

#include "serialize/serializer.hpp"
#include "serialize/context.hpp"
#include "serialize/math.hpp"
#include "serialize/type_id.hpp"

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/assert.hpp"
#include "util/optional.hpp"
#include "util/collections.hpp"
#include "util/alloc_ptr.hpp"

namespace arc = archimedes;

// concept which is true when T has a specialized serializer (should not be
// serialized via some RecordSerializer, use Serializer<T> instead)
template <typename T>
concept HasSpecializedSerializer =
    !types::is_record<T>
    || math::is_vec<T>::value
    || types::is_optional<T>::value
    || types::is_vector<T>::value
    || types::is_unique_ptr<T>::value
    || std::same_as<T, std::string>;

// serialize function for some any ptr
using SerializeAnyFn =
    std::function<void(const arc::any&, Archive&, SerializationContext&)>;

// deserialize function for some any ptr
using DeserializeAnyFn =
    std::function<void(const arc::any&, Archive&, SerializationContext&)>;

// callback method which accepts SerializationContext[, Archive]
using SerializationCallbackFn =
    std::function<void(const arc::any&, SerializationContext&, Archive&)>;

// runtime generated serializer for record fields for some type
// unique per record type INDEPENDENT of inheritance hierarchy
struct RecordFieldsSerializer {
    // serializer instances
    std::vector<std::unique_ptr<arc::any>> serializers;

    // field de/serializers
    std::vector<std::tuple<SerializeAnyFn, DeserializeAnyFn>> fs;

    static RecordFieldsSerializer create(
        const arc::reflected_record_type &type);

    void serialize(
        const arc::any &ptr,
        Archive &archive,
        SerializationContext &ctx) const {
        for (const auto &[f_ser, _]: this->fs) {
            f_ser(ptr, archive, ctx);
        }
    }

    void deserialize(
        const arc::any &ptr,
        Archive &archive,
        SerializationContext &ctx) const {
        for (const auto &[_, f_deser]: this->fs) {
            f_deser(ptr, archive, ctx);
        }
    }
};

// runtime generated record serializer for some record type
// each inheritance hierarchy NEEDS its own record serializer (due to vbases)
struct RecordSerializer {
    struct BaseSerializer {
        const arc::reflected_record_type parent, base;

        // serializer for base type
        RecordSerializer *serializer;

        BaseSerializer(
            const arc::reflected_record_type &parent,
            const arc::reflected_record_type &base)
            : parent(parent),
              base(base),
              serializer(&RecordSerializer::get_or_create(base)) {}

        // cast some ptr_parent to this base type
        arc::any cast(const arc::any &ptr_parent) const {
            // TODO: cache as ptrdiff_t
            const auto res = arc::cast(ptr_parent, this->parent, this->base);
            ASSERT(res);
            return *res;
        }

        // cast to base type and serialize
        void serialize_on_parent(
            const arc::any &ptr_parent,
            Archive &a,
            SerializationContext &ctx) const {
            this->serializer->serialize(
                this->cast(ptr_parent), a, ctx, true);
        }

        // cast to base type and deserialize
        void deserialize_on_parent(
            const arc::any &ptr_parent,
            Archive &a,
            SerializationContext &ctx) const {
            this->serializer->deserialize(
                this->cast(ptr_parent), a, ctx, true);
        }
    };

    // registered serializers
    static std::unordered_map<arc::type_id, std::unique_ptr<RecordSerializer>>
        serializers;

    // type_id of base record type that is serialized by this RecordSerializer
    arc::type_id type_id;

    // serializers for base classes, vbases
    std::vector<BaseSerializer> bases, vbases;

    // fields serializer
    RecordFieldsSerializer fields;

    // context/archive callback functions, if present on type
    std::optional<SerializationCallbackFn>
        on_before_serialize,
        on_after_serialize,
        on_before_deserialize,
        on_after_deserialize,
        on_resolve;

    // get or create record serializer by reflected type
    static inline RecordSerializer &get_or_create(
        const arc::reflected_record_type &type) {
        ASSERT(
            type.is_record(),
            "cannot get record serializer for non-record type {}",
            type.name());

        const auto it = serializers.find(type.id());
        if (it != serializers.end()) { return *it->second; }

        return
            *serializers.emplace(
                type.id(),
                std::make_unique<RecordSerializer>(
                    RecordSerializer::create(type.as_record()))).first->second;
    }

    // get or create record serializer by reflected type id
    static inline RecordSerializer &get_or_create(
        arc::type_id id) {
        const auto type =
            OPT_OR_ASSERT(
                arc::reflect(id),
                "could not reflect type id {} to get serializer",
                id.value());
        ASSERT(
            type.is_record(),
            "cannot get a record serializer for non-record type {}",
            type.name());
        return RecordSerializer::get_or_create(type.as_record());
    }

    // get or create record serializer by type
    template <typename T>
        requires std::is_class_v<T> || std::is_union_v<T>
    static inline RecordSerializer &get_or_create() {
        return RecordSerializer::get_or_create(arc::type_id::from<T>());
    }

    // make serialization schema for some reflected type
    static RecordSerializer create(const arc::reflected_record_type &type);

    template <typename T>
        requires std::is_class_v<T> || std::is_union_v<T>
    static inline RecordSerializer create() {
        const auto ref = arc::reflect<T>();
        ASSERT(
            ref,
            "could not reflect type {} to make serializer",
            arc::type_name<T>());
        return RecordSerializer::create(ref->as_record());
    }

    // serialize from ptr to record
    void serialize(
        const arc::any &ptr,
        Archive &archive,
        SerializationContext &ctx,
        bool is_base = false) const;

    // deserialize from ptr to record
    void deserialize(
        const arc::any &ptr,
        Archive &archive,
        SerializationContext &ctx,
        bool is_base = false) const;
};

// serializer for arbitary record types
// NOTE: do not instantiate directly, use a Serializer<T> specialization
template <typename T>
struct BaseRecordSerializer {
    BaseRecordSerializer()
        : rs(&RecordSerializer::get_or_create<T>()) {}

    void serialize(
        const T &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("BaseRecordSerializer<{}>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        rs->serialize(arc::any::make_reference(t), archive, ctx);
    }

    void deserialize(
        T &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("BaseRecordSerializer<{}>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        rs->deserialize(arc::any::make_reference(t), archive, ctx);
    }

private:
    RecordSerializer *rs;
};

// serializer for arbitrary record types
template <types::is_record T>
    requires (!HasSpecializedSerializer<T>)
struct [[SERIALIZER]] Serializer<T>
    : BaseRecordSerializer<T>,
      SerializerImpl<T> {
    void serialize(
        const T &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        BaseRecordSerializer<T>::serialize(t, archive, ctx);
    }

    void deserialize(
        T &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        BaseRecordSerializer<T>::deserialize(t, archive, ctx);
    }

private:
    RecordSerializer *rs;
};

// serializer for pointers to non-polymorphic record types
template <types::is_record T>
    requires (
        !std::is_polymorphic_v<T>
        && !HasSpecializedSerializer<T>)
struct [[SERIALIZER]] Serializer<T*>
    : SerializerImpl<T*> {
    void serialize(
        const T* const &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}*>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));

        Serializer<u8>().serialize(t ? 0xFF : 0x00, archive, ctx);
        if (!t) {
            return;
        }

        RecordSerializer::get_or_create<T>()
            .serialize(arc::any::make(t), archive, ctx);
    }

    void deserialize(
        T* &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}*>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        return this->deserialize_alloc(t, archive, ctx);
    }

    // special deserialize function which acceepts "allocator_out"/"alloc_out"
    // parameters which will write the allocator and ORIGINAL allocated pointer
    // out, as well as deserializing
    void deserialize_alloc(
        T *&t,
        Archive &archive,
        SerializationContext &ctx,
        Allocator **allocator_out = nullptr,
        void **alloc_out = nullptr) const {
        u8 present;
        Serializer<u8>().deserialize(present, archive, ctx);
        if (!present) {
            t = nullptr;
            return;
        }

        Allocator &allocator = ctx.allocator(arc::type_id::from<T>());
        t = allocator.alloc<T>();
        ctx.notify_alloc(arc::any::make(t), arc::type_id::from<T>());
        RecordSerializer::get_or_create<T>()
            .deserialize(arc::any::make(t), archive, ctx);

        if (allocator_out) {
            *allocator_out = &allocator;
        }

        if (alloc_out) {
            *alloc_out = t;
        }
    }
};

// serializer for pointers to polymorphic record types
template <types::is_record T>
    requires (
        std::is_polymorphic_v<T>
        && !HasSpecializedSerializer<T>)
struct [[SERIALIZER]] Serializer<T*>
    : SerializerImpl<T*> {

    void serialize(
        const T* const &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}*>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        // write 0xFF if non-null, 0x00 if nullptr
        Serializer<u8>().serialize(t ? 0xFF : 0x00, archive, ctx);
        if (!t) {
            return;
        }

        // write type_id out
        const auto type =
            OPT_OR_ASSERT(
                arc::reflect_by_typeid(typeid(*t)),
                "could not reflect {} ({}) by type id",
                arc::type_name<T>(),
                typeid(*t).name()).as_record();

        Serializer<arc::type_id>().serialize(
            type.id(),
            archive,
            ctx);

        // cast to real type
        const void *p =
            OPT_OR_ASSERT(
                arc::cast(
                    t,
                    arc::reflect<T>()->as_record(),
                    type),
                "could not cast {} to {}",
                arc::type_name<T>(),
                type.qualified_name());

        // get serializer for type_id and serialize
        RecordSerializer::get_or_create(type)
            .serialize(arc::any::make(p), archive, ctx);
    }

    void deserialize(
        T* &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}*>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        this->deserialize_alloc(t, archive, ctx);
    }

    // special deserialize function which acceepts "allocator_out"/"alloc_out"
    // parameters which will write the allocator and ORIGINAL allocated pointer
    // out, as well as deserializing
    void deserialize_alloc(
        T *&t,
        Archive &archive,
        SerializationContext &ctx,
        Allocator **allocator_out = nullptr,
        void **alloc_out = nullptr) const {
        // check for nullptr
        u8 present;
        Serializer<u8>().deserialize(present, archive, ctx);
        if (!present) {
            t = nullptr;
            return;
        }

        // read type_id in and use default constructor to create value
        arc::type_id id;
        Serializer<arc::type_id>().deserialize(
            id,
            archive,
            ctx);
        const arc::reflected_record_type rec =
            OPT_OR_ASSERT(
                arc::reflect(id),
                "could not reflect for {} (read id {})",
                arc::type_name<T>(),
                id.value()).as_record();
        ASSERT(rec.is_record());
        const arc::reflected_function default_ctor =
            OPT_OR_ASSERT(
                rec.default_constructor(),
                "could not get default ctor for {}",
                arc::type_name<T>());

        Allocator &allocator = ctx.allocator(id);
        void *ptr = allocator.alloc(rec.size());

        SERIALIZE_LOG(
            "invoking default ctor {} on {}",
            default_ctor.qualified_name(),
            fmt::ptr(t));

        default_ctor.invoke(ptr);

        ctx.notify_alloc(arc::any::make(ptr), id);

        // perform proper cast to return pointer
        t =
            reinterpret_cast<T*>(
                OPT_OR_ASSERT(
                    arc::cast(
                        ptr,
                        rec,
                        arc::reflect<T>()->as_record()),
                    "could not cast {} to {}",
                    rec.qualified_name(),
                    arc::type_name<T>()));

        // get serializer for type_id and deserialize
        // deserialize on ptr (not t!) so "real" type is used
        RecordSerializer::get_or_create(rec)
            .deserialize(arc::any::make(ptr), archive, ctx);

        // write out params
        if (allocator_out) {
            *allocator_out = &allocator;
        }

        if (alloc_out) {
            *alloc_out = ptr;
        }
    }
};

// serializer for alloc_ptr which returns alloc_ptr where _alloc is right even
// for polymorphic types
// TODO: doc better
template <typename T>
struct [[SERIALIZER]] Serializer<alloc_ptr<T>>
    : SerializerImpl<alloc_ptr<T>>,
      Serializer<T*> {
    using Base = Serializer<T*>;

    void serialize(
        const alloc_ptr<T> &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG(
            "(alloc_ptr) Serializer<std::alloc_ptr<{}>>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        Base::serialize(t.get(), archive, ctx);
    }

    void deserialize(
        alloc_ptr<T> &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG(
            "(alloc_ptr) Serializer<std::alloc_ptr<{}>>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));

        Allocator *allocator = nullptr;
        void *alloc = nullptr;
        T *ptr = nullptr;
        Base::deserialize_alloc(ptr, archive, ctx, &allocator, &alloc);
        t = alloc_ptr<T>(ptr, allocator, alloc);
    }
};

// serializer for unique_ptrs
// specializes based on whether or not underlying type is polymorphic or not
// TODO: doc better
template <typename T>
struct [[SERIALIZER]] Serializer<std::unique_ptr<T>>
    : SerializerImpl<std::unique_ptr<T>>,
      Serializer<std::conditional_t<std::is_polymorphic_v<T>, T*, T>> {
    using Base =
        Serializer<std::conditional_t<std::is_polymorphic_v<T>, T*, T>>;

    void serialize(
        const std::unique_ptr<T> &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG(
            "(unique_ptr) Serializer<std::unique_ptr<{}>>::serialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));
        if constexpr (std::is_polymorphic_v<T>) {
            Base::serialize(t.get(), archive, ctx);
        } else {
            Serializer<u8>().serialize(t ? 0xFF : 0x00, archive, ctx);
            if (t) {
                Base::serialize(*t, archive, ctx);
            }
        }
    }

    void deserialize(
        std::unique_ptr<T> &t,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG(
            "(unique_ptr) Serializer<std::unique_ptr<{}>>::deserialize @ {}",
            arc::type_name<T>(),
            fmt::ptr(&t));

        if constexpr (std::is_polymorphic_v<T>) {
            T *ptr;
            Base::deserialize(ptr, archive, ctx);
            t.reset(ptr);
        } else {
            u8 present;
            Serializer<u8>().deserialize(present, archive, ctx);
            if (present) {
                t = std::make_unique<T>();
                Base::deserialize(*t, archive, ctx);
            }
        }
    }
};
