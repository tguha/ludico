#include "serialize/record.hpp"

// static storage
decltype(RecordSerializer::serializers) RecordSerializer::serializers;

// find all serializers (all structs marked with [[SERIALIZER]])
static std::vector<arc::reflected_record_type> get_serializers() {
    return collections::copy_to<std::vector<arc::reflected_record_type>>(
        arc::reflect_by_annotations(
            types::make_array(
                std::string_view(SERIALIZER_TEXT))),
        [](const arc::reflected_type &type) {
            return type.as_record();
        });
}

// gets type which some serializer serializes, either by:
// * using its Serializer<T> specialization, in which case function returns T
// * using its SerializerImpl<T> base, in which case function returns T
// * using its SerializedType member type
static std::optional<arc::reflected_type> get_serializer_type(
    arc::reflected_record_type type) {
    if (type.name() == "Serializer"
            && type.template_parameters().size() == 1) {
        const auto tp = type.template_parameters()[0];
        return tp.is_typename() ? std::make_optional(*tp.type()) : std::nullopt;
    } else if (
        const auto si_base =
            type.base(".*SerializerImpl.*")) {
        ASSERT(
            si_base->type().kind() == arc::STRUCT,
            "SerializerImpl on {} not properly reflected",
            type.qualified_name());
        ASSERT(
            si_base->type().template_parameters().size() >= 1,
            "malformed SerializerImpl {}",
            si_base->type().qualified_name());
        const auto tp = si_base->type().template_parameters()[0];
        return tp.is_typename() ? std::make_optional(*tp.type()) : std::nullopt;
    } else if (
        const auto ta =
            type.type_alias("SerializedType")) {
        return *ta->type();
    }

    return std::nullopt;
}

// try to find serializer for type id
// also respects SERIALIZE_AS(...) annotations
inline std::optional<arc::reflected_record_type> serializer_for_type(
    arc::type_id id) {
    // check for SERIALIZE_AS
    const auto type = archimedes::reflect(id);
    ASSERT(type, "cannot find serializer for invalid id {}", id.value());
    if (type->is_record()) {
        const auto as = type->annotations();
        if (const auto a = collections::find(
                type->annotations(),
                [](const auto &a) {
                    return a.starts_with(SERIALIZE_AS_TEXT);
                })) {
            SERIALIZE_LOG("got serialize as for {}", type->name());
            const auto other =
                a->substr(
                    std::string_view(SERIALIZE_AS_TEXT).length());

            const auto other_type = archimedes::reflect(other);
            ASSERT(
                other_type,
                "cannot SERIALIZE_AS non reflected type {}",
                other);

            // lookup serializer for other type
            return serializer_for_type(other_type->id());
        }
    }

    const auto ss = get_serializers();

    // find with matching type id
    auto it = std::find_if(
        ss.begin(),
        ss.end(),
        [&](const arc::reflected_record_type &s) {
            ASSERT(
                get_serializer_type(s),
                "malformed serializer {}",
                s.name());

            const auto impl =
                OPT_OR_ASSERT(
                    s.base("SerializerImpl.*"),
                    "serializer {} does not inherit from SerializerImpl",
                    s.name()).type();
            ASSERT(
                impl.kind() != arc::UNKNOWN,
                "could not reflect SerializerImpl for {}",
                s.qualified_name());

            const auto serialize_type_id =
                OPT_OR_ASSERT(
                    impl.static_field("serialize_type_id"),
                    "could not get serialize_type_id on impl {}",
                    impl.qualified_name());

            ASSERT(
                serialize_type_id.constexpr_value(),
                "no constexpr_value for field {} on {}",
                serialize_type_id.name(),
                impl.qualified_name());

            return
                serialize_type_id.constexpr_value()->as<arc::type_id>()
                    == id;
        });

    return it != ss.end() && it->is_record() ?
        std::make_optional(it->as_record())
        : std::nullopt;
}

// make two serialize functions for the specified type which expect a pointer to
// that type
static std::optional<std::tuple<SerializeAnyFn, DeserializeAnyFn>>
    make_functions_for_type(
        RecordFieldsSerializer &rfs,
        arc::reflected_record_type st) {

    const auto serialized_type =
        OPT_OR_ASSERT(
            get_serializer_type(st),
            "could not get serialized type for serializer {}",
            st.qualified_name());

    const auto fs_serialize =
        OPT_OR_ASSERT(
            st.function_set("serialize"),
            "cannot get serialize function set for type {}",
            st.name());

    // find matching serialize function
    const auto f_serialize =
        OPT_OR_ASSERT(
            collections::find(
                fs_serialize,
                [&](const arc::reflected_function &f) {
                    const auto ps = f.type().parameters();
                    return ps.size() == 3
                        && ps[0].type().kind() == arc::REF
                        && **ps[0].type().type() == serialized_type;
                }),
            "no matching serialize function on {}",
            st.name());
    ASSERT(
        f_serialize.can_invoke(),
        "cannot invoke serialize function for {}",
        st.name());

    const auto fs_deserialize =
        OPT_OR_ASSERT(
            st.function_set("deserialize"),
            "cannot get deserialize function set for type {}",
            st.name());

    // find matching deserialize function
    const auto f_deserialize =
        OPT_OR_ASSERT(
            collections::find(
                fs_deserialize,
                [&](const arc::reflected_function &f) {
                    const auto ps = f.type().parameters();
                    return ps.size() == 3
                        && ps[0].type().kind() == arc::REF
                        && **ps[0].type().type() == serialized_type;
                }),
            "no matching deserialize function on {}",
            st.name());
    ASSERT(
        f_deserialize.can_invoke(),
        "cannot invoke deserialize function for {}",
        st.name());

    SERIALIZE_LOG("using functions {}/{}, {}/{} for type serializer {}/{}",
        f_serialize.qualified_name(),
        f_serialize.type().name(),
        f_deserialize.qualified_name(),
        f_deserialize.type().name(),
        st.qualified_name(),
        serialized_type.name());

    const auto res = arc::any::make_for_type(st);
    if (!res) { return std::nullopt; }

    // TODO: clean
    rfs.serializers.push_back(std::make_unique<arc::any>(*res));
    const auto *p_instance =
        rfs.serializers[rfs.serializers.size() - 1]->storage();

    return std::make_tuple<SerializeAnyFn, DeserializeAnyFn>(
        [p_instance, f_serialize](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("serializing type {} @ {}",
                f_serialize.parameters()[0].type()->name(),
                fmt::ptr(ptr.as<void*>()));
            OPT_OR_ASSERT(
                f_serialize.invoke(
                    p_instance,
                    ptr.as<void*>(),
                    arc::ref(a),
                    arc::ref(ctx)),
                "failed to invoke f_serialize defined at {}",
                f_serialize.definition_path());
        },
        [p_instance, f_deserialize](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("deserializing type {} @ {}",
                f_deserialize.parameters()[0].type()->name(),
                fmt::ptr(ptr.as<void*>()));

            OPT_OR_ASSERT(
                f_deserialize.invoke(
                    p_instance,
                    ptr.as<void*>(),
                    arc::ref(a),
                    arc::ref(ctx)),
                "failed to invoke f_deserialize defined at {}",
                f_deserialize.definition_path());
        });
}

// TODO: doc
static std::optional<std::tuple<SerializeAnyFn, DeserializeAnyFn>>
        make_serializers_for_field(
    RecordFieldsSerializer &rfs,
    const arc::reflected_field &field) {
    const auto st = TRY_OPT(serializer_for_type(field.type().id()));
    const auto [f_ser, f_deser] =
        TRY_OPT(make_functions_for_type(rfs, st));

    return std::make_tuple<SerializeAnyFn, DeserializeAnyFn>(
        [field, f_ser = f_ser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("serializing field {}/{} @ {} on {}",
                field.qualified_name(),
                field.type()->name(),
                fmt::ptr(*field.get_ptr_from_any_ptr<void>(ptr)),
                fmt::ptr(ptr.as<void*>()));
            f_ser(
                *field.get_any_ptr_from_any_ptr(ptr),
                a,
                ctx);
        },
        [field, f_deser = f_deser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("deserializing field {}/{} @ {} on {}",
                field.qualified_name(),
                field.type()->name(),
                fmt::ptr(*field.get_ptr_from_any_ptr<void>(ptr)),
                fmt::ptr(ptr.as<void*>()));
            f_deser(
                *field.get_any_ptr_from_any_ptr(ptr),
                a,
                ctx);
        });
}

// TODO: doc
static std::tuple<SerializeAnyFn, DeserializeAnyFn>
    make_serialize_by_id(
        RecordFieldsSerializer &rfs,
        arc::reflected_field field,
        std::string_view annotation) {
    const auto args =
        collections::ctransform(
            strings::split(
                annotation.substr(std::strlen(SERIALIZE_BY_ID_TEXT)),
                ","),
            strings::trim);
    ASSERT(
        args.size() == 2,
        "expected 2 arguments for SERIALIZE_BY_ID, got {}",
        args.size());

    const auto
        context_name = args[0],
        id_field_name = args[1];

    // field must be pointer or reference type
    ASSERT(field.type()->kind() == arc::PTR
        || field.type()->kind() == arc::REF,
        "cannot SERIALIZE_BY_ID on non-ptr/ref field {}",
        field.qualified_name());

    // get id field
    const auto id_field =
        OPT_OR_ASSERT(
            arc::reflect_field(id_field_name),
            "could not find id field {}",
            id_field_name);

    // get "lookup_id" function off of serialization context
    const auto r_ctx_opt = arc::reflect(context_name);
    ASSERT(
        r_ctx_opt, "cannot reflect ctx {}", context_name);
    ASSERT(
        r_ctx_opt->is_record(), "ctx {} is not record", context_name);

    // get context type
    const auto ctx_type = r_ctx_opt->as_record();

    const auto lookup_id_fns = ctx_type.function_set("lookup_id");
    ASSERT(
        lookup_id_fns,
        "no function lookup_id on {}",
        context_name);

    // find function where paremter is ID field type AND return type
    // exactly matches that of field
    auto lookup_id_fn =
        OPT_OR_ASSERT(
            collections::find(
                *lookup_id_fns,
                [&](const arc::reflected_function &f) {
                    const auto ps = f.parameters();
                    return ps.size() == 1
                        && ps[0].type() == id_field.type()
                        && f.type().return_type() == field.type();
                }),
            "could not find lookup_id with correct type on {} for field {}",
            ctx_type.name(),
            field.qualified_name());

    // get de/serializer functions for ID field type
    // NOTE: not field, just type - pointers are directly to value
    const auto serializer_id_type =
        OPT_OR_ASSERT(
            serializer_for_type(
                id_field.type().id()),
            "could not get serializer for if field {} {}",
            id_field.type()->name(),
            id_field.name());

    const auto [f_id_ser, f_id_deser] =
        OPT_OR_ASSERT(
            make_functions_for_type(rfs, serializer_id_type),
            "could not make serializers for field {}",
            id_field.qualified_name());

    return std::make_tuple<SerializeAnyFn, DeserializeAnyFn>(
        [field, id_field, f_id_ser = f_id_ser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("serializing id field {}/{} on type {} @ {}",
                id_field.qualified_name(),
                id_field.type()->name(),
                field.parent().qualified_name(),
                fmt::ptr(ptr.as<void*>()));

            // field is guaranteed to be ref/pointer: get value
            const void *field_value =
                *field.get_from_any_ptr<void*>(ptr);

            // write "present" byte
            Serializer<u8>().serialize(
                field_value ? 0xFF : 0x00, a, ctx);

            if (field_value) {
                // write ID field out
                SERIALIZE_LOG("writing id {}",
                    *reinterpret_cast<u64*>(
                        id_field.get_any_ptr(field_value)->storage()));
                f_id_ser(*id_field.get_any_ptr(field_value), a, ctx);
            }
        },
        [ctx_type, lookup_id_fn, field, id_field, f_id_deser = f_id_deser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("deserializing id field {}/{} on type {} @ {}",
                id_field.qualified_name(),
                id_field.type()->name(),
                field.parent().qualified_name(),
                fmt::ptr(ptr.as<void*>()));

            // guaranteed to be ptr/ref type
            void **field_ptr = *field.get_ptr_from_any_ptr<void*>(ptr);

            // read "present" byte
            u8 present;
            Serializer<u8>().deserialize(present, a, ctx);

            // not present -> original value was nullptr
            if (!present) {
                *field_ptr = nullptr;
                return;
            }

            // read ID field into temporary value
            const auto id =
                OPT_OR_ASSERT(arc::any::make_for_type(*id_field.type()));
            f_id_deser(id.ptr(), a, ctx);
            SERIALIZE_LOG("deserialized id {}",
                    *reinterpret_cast<u64*>(id.storage()));

            // cast context to target ctx type
            const auto cast_ctx =
                OPT_OR_ASSERT(
                    arc::cast(
                        &ctx,
                        *arc::reflect<SerializationContext>(),
                        ctx_type),
                    "could not cast ctx of type {} to {}",
                    typeid(ctx_type).name(),
                    ctx_type.name());

            // enqueue resolution
            ctx.queue_resolve(
                [id, field_ptr, lookup_id_fn, cast_ctx]() {
                    SERIALIZE_LOG("looking up id {}",
                        *reinterpret_cast<u64*>(id.storage()));

                    // call lookup id function on cast_ctx
                    const auto looked_up =
                        OPT_OR_ASSERT(
                            lookup_id_fn.invoke_with(
                                types::make_array(
                                    arc::any::make(cast_ctx),
                                    id)),
                            "failed to invoke {}",
                            lookup_id_fn.name());

                    // assign to field
                    std::memcpy(
                        field_ptr,
                        &looked_up.bytes()[0],
                        looked_up.size());
                });
        }
    );
}

static std::tuple<SerializeAnyFn, DeserializeAnyFn>
    make_serialize_by_ctx(
        RecordFieldsSerializer &rfs,
        arc::reflected_field field,
        std::string_view annotation) {
    const auto args =
        collections::ctransform(
            strings::split(
                annotation.substr(std::strlen(SERIALIZE_BY_CTX_TEXT)),
                ","),
            strings::trim);
    ASSERT(args.size() == 2, "expected 2 args for SERIALIZE_BY_CTX, got {}");

    std::optional<SerializeAnyFn> fn_ser = std::nullopt;
    std::optional<DeserializeAnyFn> fn_deser = std::nullopt;

    // split an argument in the form Record::function into its constituent
    // (record, function) reflections
    const auto get_ctx_fn =
        [](const auto &arg) {
            const auto pos_qual = arg.find("::");
            const auto
                ctx_name = arg.substr(0, pos_qual),
                fn_name = arg.substr(pos_qual + 2);

            const auto ctx_type =
                OPT_OR_ASSERT(
                    arc::reflect(ctx_name),
                    "could not reflect {}",
                    ctx_name);

            ASSERT(
                ctx_type.is_record(),
                "{} is not record",
                ctx_type.name());

            const auto ctx_rec = ctx_type.as_record();

            const auto fn =
                OPT_OR_ASSERT(
                    ctx_rec.function(fn_name),
                    "no such function {} on {}",
                    fn_name,
                    ctx_name);

            return std::make_tuple(ctx_rec, fn);
        };

    if (args[0] != "SERIALIZE_IGNORE") {
        const auto [ctx_type, fn] = get_ctx_fn(args[0]);

        ASSERT(
            fn.parameters().size() == 1,
            "function {} must only have 1 parameter",
            fn.qualified_name());

        fn_ser =
            [field, ctx_type = ctx_type, fn = fn](
                const arc::any &ptr,
                Archive &a,
                SerializationContext &ctx) {
                SERIALIZE_LOG(
                    "serializing by ctx field {}/{} on type {} @ {} with {}",
                    field.qualified_name(),
                    field.type()->name(),
                    field.parent().qualified_name(),
                    fmt::ptr(ptr.as<void*>()),
                    fn.qualified_name());

                // get pointer to field
                const auto field_ptr =
                    *field.get_any_ptr_from_any_ptr(ptr);

                // cast context to target ctx type
                const auto cast_ctx =
                    OPT_OR_ASSERT(
                        arc::cast(
                            &ctx,
                            *arc::reflect<SerializationContext>(),
                            ctx_type),
                        "could not cast ctx of type {} to {}",
                        typeid(ctx).name(),
                        ctx_type.name());

                // invoke function
                OPT_OR_ASSERT(
                    fn.invoke(cast_ctx, field_ptr, arc::ref(a)),
                    "failed to invoke {}",
                    fn.qualified_name());
            };
    }

    if (args[1] != "SERIALIZE_IGNORE") {
        const auto [ctx_type, fn] = get_ctx_fn(args[1]);

        ASSERT(
            fn.parameters().size() == 0,
            "function {} has >0 parameters",
            fn.qualified_name());

        // check return type but don't worry about qualifiers - assigning a
        // const T* -> T* and vice versa won't matter as it's just a raw memcpy
        // TODO: some sort of is_assignable_from(...) for qualified types
        /* const auto return_type = fn.type().return_type(); */
        /* ASSERT( */
        /*     return_type == field.type(), */
        /*     "function return type {} != field type {}", */
        /*     fn.type().return_type().type().name(), */
        /*     field.type().type().name()); */

        fn_deser =
            [field, ctx_type = ctx_type, fn = fn](
                const arc::any &ptr,
                Archive &a,
                SerializationContext &ctx) {
                SERIALIZE_LOG(
                    "deserializing by ctx field {}/{} on type {} @ {} with {}",
                    field.qualified_name(),
                    field.type()->name(),
                    field.parent().qualified_name(),
                    fmt::ptr(ptr.as<void*>()),
                    fn.qualified_name());

                // get pointer to field
                const auto field_ptr =
                    *field.get_any_ptr_from_any_ptr(ptr);

                // cast context to target ctx type
                const auto cast_ctx =
                    OPT_OR_ASSERT(
                        arc::cast(
                            &ctx,
                            *arc::reflect<SerializationContext>(),
                            ctx_type),
                        "could not cast ctx of type {} to {}",
                        typeid(ctx).name(),
                        ctx_type.name());

                // get function result
                const auto res =
                    OPT_OR_ASSERT(
                        fn.invoke(cast_ctx),
                        "failed to invoke {}",
                        fn.qualified_name());

                // copy ito field
                std::memcpy(
                    field_ptr.as<void*>(),
                    &res.bytes()[0],
                    field.type()->size());
            };
    }

    return std::make_tuple<SerializeAnyFn, DeserializeAnyFn>(
        fn_ser ?
            *fn_ser
            : [](const arc::any&, Archive&, SerializationContext&) {},
        fn_deser ?
            *fn_deser
            : [](const arc::any&, Archive&, SerializationContext&) {});
}

static std::tuple<SerializeAnyFn, DeserializeAnyFn>
    make_serialize_custom(
        RecordFieldsSerializer &rfs,
        arc::reflected_field field,
        std::string_view annotation) {
    const auto arg = annotation.substr(std::strlen(SERIALIZE_CUSTOM_TEXT));
    const auto st =
        OPT_OR_ASSERT(
            arc::reflect(arg),
            "could not reflect custom serializer {} for {}",
            arg, field.qualified_name());
    ASSERT(
        st.is_record(),
        "custom serializer {} for {} is not record",
        arg, field.qualified_name());

    SERIALIZE_LOG("custom serializer {}, for {}/{}",
        st.as_record().qualified_name(),
        field.qualified_name(),
        field.type()->name());
    const auto [f_ser, f_deser] =
        OPT_OR_ASSERT(
            make_functions_for_type(rfs, st.as_record()));

    return std::make_tuple<SerializeAnyFn, DeserializeAnyFn>(
        [field, f_ser = f_ser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("serializing by custom field {}/{} on type {} @ {}",
                field.qualified_name(),
                field.type()->name(),
                field.parent().qualified_name(),
                fmt::ptr(ptr.as<void*>()));
            f_ser(
                *field.get_any_ptr_from_any_ptr(ptr),
                a,
                ctx);
        },
        [field, f_deser = f_deser](
            const arc::any &ptr,
            Archive &a,
            SerializationContext &ctx) {
            SERIALIZE_LOG("deserializing by custom field {}/{} on type {} @ {}",
                field.qualified_name(),
                field.type()->name(),
                field.parent().qualified_name(),
                fmt::ptr(ptr.as<void*>()));
            f_deser(
                *field.get_any_ptr_from_any_ptr(ptr),
                a,
                ctx);
        });
}

void RecordSerializer::serialize(
    const arc::any &ptr,
    Archive &archive,
    SerializationContext &ctx,
    bool is_base) const {
    SERIALIZE_LOG("RecordSerializer::serialize for {} @ {}",
        arc::reflect(this->type_id)->name(),
        fmt::ptr(ptr.as<void*>()));
    if (this->on_before_serialize) {
        (*this->on_before_serialize)(ptr, ctx, archive);
    }

    for (const auto &base: this->bases) {
        SERIALIZE_LOG(
            "invoking base serializer {} : {}",
            base.parent.qualified_name(),
            base.base.qualified_name());
        base.serialize_on_parent(ptr, archive, ctx);
    }

    if (!is_base) {
        for (const auto &vbase : this->vbases) {
            SERIALIZE_LOG(
                "invoking vbase serializer {} : {}",
                vbase.parent.qualified_name(),
                vbase.base.qualified_name());
            vbase.serialize_on_parent(ptr, archive, ctx);
        }
    }

    fields.serialize(ptr, archive, ctx);
    ctx.notify_serialize(ptr, this->type_id);

    if (this->on_after_serialize) {
        (*this->on_after_serialize)(ptr, ctx, archive);
    }
}

void RecordSerializer::deserialize(
    const arc::any &ptr,
    Archive &archive,
    SerializationContext &ctx,
    bool is_base) const {
    SERIALIZE_LOG("RecordSerializer::deserialize for {} @ {}",
        arc::reflect(this->type_id)->name(),
        fmt::ptr(ptr.as<void*>()));
    if (this->on_before_deserialize) {
        (*this->on_before_deserialize)(ptr, ctx, archive);
    }

    for (const auto &base : this->bases) {
         base.deserialize_on_parent(ptr, archive, ctx);
    }

    if (!is_base) {
        for (const auto &vbase : this->vbases) {
            vbase.deserialize_on_parent(ptr, archive, ctx);
        }
    }

    fields.deserialize(ptr, archive, ctx);
    ctx.notify_deserialize(ptr, this->type_id);

    if (this->on_resolve) {
        ctx.queue_resolve(SerializationContext::ResolveFn(
            [f = *this->on_resolve, ptr, &ctx, &archive]() {
                f(ptr, ctx, archive);
            }));
    }

    if (this->on_after_deserialize) {
        (*this->on_after_deserialize)(ptr, ctx, archive);
    }
}

RecordFieldsSerializer RecordFieldsSerializer::create(
    const arc::reflected_record_type &type) {
    RecordFieldsSerializer rfs;

    for (const auto field : type.fields()) {
        // check annotations
        bool custom = false;
        const auto as = field.annotations();
        for (const auto &a : as) {
            if (custom) { break; }
            if (a.starts_with(SERIALIZE_IGNORE_TEXT)) {
                custom = true;
            } else if (a.starts_with(SERIALIZE_BY_ID_TEXT)) {
                rfs.fs.emplace_back(
                    make_serialize_by_id(rfs, field, a));
                custom = true;
            } else if (a.starts_with(SERIALIZE_CUSTOM_TEXT)) {
                rfs.fs.emplace_back(
                    make_serialize_custom(rfs, field, a));
                custom = true;
            } else if (a.starts_with(SERIALIZE_BY_CTX_TEXT)) {
                rfs.fs.emplace_back(
                    make_serialize_by_ctx(rfs, field, a));
                custom = true;
            }
        }

        // don't handle this field manually if it is taken by some annotation
        if (custom) {
            continue;
        }

        // to serialize a record field which is polymorphic we require that
        // it is annotated with SERIALIZE_POLY to ensure that the end user
        // is aware that this field will NOT respect its polymorphic/virtual
        // type when being de/serialized.
        if (field.type()->is_record()
                && field.type()->as_record().is_polymorphic()) {
            ASSERT(
                collections::find(as, SERIALIZE_POLY_TEXT),
                "cannot serialize polymorphic field {} on record {} unless"
                " marked explicitly with SERIALIZE_POLY",
                field.name(),
                type.name());
        }

        // check first if there's some primitive Serializer<T> for this type -
        // doesn't matter if it's a pointer, unknown, etc. if we have a
        // serializer we can use it.
        if (const auto serializer =
                serializer_for_type(field.type()->id())) {

            // have a primitive serializer - use it. preferred over synthesized
            // RecordSerializers
            const auto fs =
                OPT_OR_ASSERT(
                    make_serializers_for_field(rfs, field),
                    "could not make serializers for field {}",
                    field.qualified_name());
            rfs.fs.emplace_back(fs);
        } else if (field.type()->is_record()) {
            // create RecordSerializer
            auto *serializer =
                &RecordSerializer::get_or_create(field.type()->as_record());
            rfs.fs.emplace_back(
                [field, serializer](
                    const arc::any &any,
                    Archive &a,
                    SerializationContext &ctx) {
                    serializer->serialize(
                        *field.get_any_ptr_from_any_ptr(any), a, ctx);
                },
                [field, serializer](
                    const arc::any &any,
                    Archive &a,
                    SerializationContext &ctx) {
                    serializer->deserialize(
                        *field.get_any_ptr_from_any_ptr(any), a, ctx);
                });
        } else {
            // error: no primitive serializer and could not construct
            // synthesized serializer
            ASSERT(
                false,
                "no primitive and could not construct synthesized for"
                " {} ({}/kind: {})",
                field.name(),
                field.type()->name(),
                field.type()->kind());
        }
    }
    return rfs;
}

RecordSerializer RecordSerializer::create(
    const arc::reflected_record_type &type) {
    ASSERT(
        type.default_constructor(),
        "cannot find default constructor to serialize type {}",
        type.name());

    // get a function in style of
    // "<name>(<child of SerialiationContext> &[, Archive&])"
    // on the record type
    const auto get_callback =
        [&](std::string_view name) -> std::optional<SerializationCallbackFn> {
            // TODO: check that parameter is some SerializationContext*
            const auto fn =
                TRY_OPT(
                    collections::find(
                        type.functions(),
                        [&](const arc::reflected_function &f) {
                            return
                                f.name() == name
                                && f.is_member()
                                && f.parameters().size() >= 1
                                && f.parameters()[0].type()->kind() == arc::REF;
                        }));

            const auto ps = fn.parameters();
            ASSERT(
                ps.size() == 1 || ps.size() == 2,
                "incorrect callback type for {}, {}",
                fn.qualified_name(),
                fn.type().name());

            const auto ctx_type = **ps[0].type()->type();
            ASSERT(
                ctx_type.is_record(),
                "function {} context parameter type {} is not record",
                fn.qualified_name(),
                ctx_type.name());

            // if there is a second parameter ensure it is Archive&
            if (ps.size() == 2) {
                // TODO: deeper checking of Archive&-ness
                ASSERT(
                    ps[1].type()->kind() == arc::REF,
                    "invalid Archive& parameter {} on callback {}",
                    ps[1].name(),
                    fn.qualified_name());
            }

            const auto n_parameters = fn.parameters().size();
            return
                [fn, ctx_type, n_parameters](
                    const arc::any &ptr,
                    SerializationContext &ctx,
                    Archive &archive) {
                    void *cast_ctx =
                        OPT_OR_ASSERT(
                            arc::cast(
                                &ctx,
                                *arc::reflect<SerializationContext>(),
                                ctx_type.as_record()),
                            "could not cast {} to {}",
                            typeid(ctx).name(),
                            ctx_type.name());

                    if (n_parameters == 1) {
                        fn.invoke(ptr.as<void*>(), cast_ctx);
                    } else {
                        fn.invoke(ptr.as<void*>(), cast_ctx, arc::ref(archive));
                    }
                };
        };

    RecordSerializer rs;
    SERIALIZE_LOG("making RecordSerializer for {}", type.qualified_name());
    rs.type_id = type.id();
    rs.on_resolve = get_callback("on_resolve");
    rs.on_before_serialize = get_callback("on_before_serialize");
    rs.on_after_serialize = get_callback("on_after_serialize");
    rs.on_before_deserialize = get_callback("on_before_deserialize");
    rs.on_after_deserialize = get_callback("on_after_deserialize");
    rs.fields = RecordFieldsSerializer::create(type);

    for (const auto &base : type.bases()) {
        SERIALIZE_LOG(
            "adding base {} to {}",
            base.type().qualified_name(),
            type.qualified_name());
        rs.bases.emplace_back(BaseSerializer(type, base.type()));
    }

    for (const auto &vbase : type.vbases()) {
        SERIALIZE_LOG(
            "adding vbase {} to {}",
            vbase.type().qualified_name(),
            type.qualified_name());
        rs.vbases.emplace_back(BaseSerializer(type, vbase.type()));
    }

    return rs;
}
