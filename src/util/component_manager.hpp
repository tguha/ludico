#pragma once

#include "util/bitset.hpp"
#include "util/bitset_128.hpp"
#include "util/types.hpp"
#include "util/type_registry.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"
#include "util/optional.hpp"
#include "ext/inplace_function.hpp"

struct Level;

// implement manager-specific methods via template
enum ComponentManagerType {
    ENTITY,
    TILE,
    ITEM
};

// implementation of a generalized component system ('ECS') for some type T with
// index type I
template <
    typename T,
    typename I,
    typename ObjBase,
    typename CompBase,
    typename CompTypeBase,
    usize MAX_COMPONENTS,
    usize MAX_OBJECTS,
    ComponentManagerType Type>
struct ComponentManager {
    // base object type (i.e. Entity in an ECS)
    struct Object : public ObjBase {
        I id = 0;

        Object() = default;
        Object(T *p, I id) : id(id), p(p) {}

        inline operator I() const {
            return this->id;
        }

        inline bool operator==(const Object &rhs) const {
            return this->id == rhs.id;
        }

        inline auto operator<=>(const Object &rhs) const {
            return this->id <=> rhs.id;
        }

        inline T &parent() const {
            return *this->p;
        }

        template <typename C>
        inline C &get() const {
            return this->p->template component<C>(*this);
        }

        template <typename C>
        inline std::optional<C*> opt() const {
            return this->p->template opt_component<C>(*this);
        }

        template <typename C>
        inline bool has() const {
            return this->p->template has_component<C>(*this);
        }

        template <typename C>
        inline C &add(C &&component = C()) const {
            static_assert(std::is_base_of_v<BaseComponent, C>);
            return this->p->add_component(*this, std::forward<C>(component));
        }

        template <typename C>
        inline void remove() const {
            return this->p->template remove_component<C>(*this);
        }

        template <typename _ = T>
            requires (Type == ENTITY)
        inline void destroy() const {
            this->p->enqueue_destroy(*this);
        }

        template <typename _ = T>
            requires (Type != ENTITY)
        inline void destroy() const {
            this->p->destroy(*this);
        }

        template <typename _ = T>
            requires (Type == ENTITY)
        inline auto &level() const {
            return *this->p->level;
        }

        inline auto &signature() const {
            return this->p->signatures[this->id];
        }

        inline std::string to_string() const {
            std::string prefix;
            if constexpr (Type == ENTITY) {
                prefix = "Entity";
            } else if constexpr (Type == ITEM) {
                prefix = "Item";
            } else if constexpr (Type == TILE) {
                prefix = "Tile";
            }
            return prefix + "(" + std::to_string(id) + ")";
        }
private:
        T *p = nullptr;
    };

    // information for a specific component type
    struct ComponentType : public CompTypeBase {
        u64 id;
        usize size;

        template <typename C>
        static inline ComponentType from(ComponentManager *system, u64 id) {
            ComponentType result;
            result.id = id;

            if constexpr (requires { C::size(); }) {
                result.size = C::size();
                static_assert(C::size() >= (sizeof(C) + alignof(C)));
            } else {
                result.size = sizeof(C) + alignof(C);
            }

            return result;
        }
    };

    // base class for all components
    struct BaseComponent : public CompBase {
        u8 block = 0;

        // components are non-copyable
        BaseComponent() = default;
        BaseComponent(const BaseComponent &other) = delete;
        BaseComponent(BaseComponent &&other) = default;
        BaseComponent &operator=(const BaseComponent &other) = delete;
        BaseComponent &operator=(BaseComponent &&other) = default;
        virtual ~BaseComponent() = default;

        // run on component addition
        virtual void init() {};
    };

    // template-specified component - extend as implementation for components,
    // i.e. PositionComponent : public Component<PositionComponent>
    template <typename C>
    struct Component : public BaseComponent {
        static u64 _id;
        static T *_p;

        inline Object object() const {
            return this->parent().template object_from_component<C>(
                reinterpret_cast<const void *>(this));
        }

        inline T &parent() const {
            return *Component<C>::_p;
        }

        template <typename _ = T>
            requires (Type == ENTITY)
        inline Object entity() const {
            return this->object();
        }

        template <typename _ = T>
            requires (Type == ENTITY)
        inline auto &level() const {
            return *this->parent().level;
        }

        template <typename _ = T>
            requires (Type == TILE)
        inline Object tile() const {
            return this->object();
        }

        template <typename _ = T>
            requires (Type == ITEM)
        inline Object item() const {
            return this->object();
        }
    };

    // an array of components of a specific type, made of up blocks of size
    // BLOCK_SIZE internally. this is preferred to realloc-ing as everything
    // keeps a consistent position in memory throughout the entire lifecycle
    // of the component manager.
    struct ComponentArray {
        static constexpr auto BLOCK_SIZE = 512;
        static constexpr auto NUM_BLOCKS = (MAX_OBJECTS / BLOCK_SIZE) + 1;

        // one block of BLOCK_SIZE components
        struct Block {
            const ComponentArray *parent = nullptr;
            std::unique_ptr<u8[]> data = nullptr;

            Block() = default;

            Block(const ComponentArray *parent, usize n)
                : parent(parent) {
                const auto data_size = BLOCK_SIZE * this->parent->type->size;

                this->data = std::unique_ptr<u8[]>(
                    new (std::align_val_t(16)) u8[data_size]);

                std::memset(data.get(), 0, data_size);
            }

            // component access via pointer
            inline BaseComponent *operator[](usize i) const {
                ASSERT(i < BLOCK_SIZE);
                return
                    reinterpret_cast<BaseComponent*>(
                        reinterpret_cast<u8*>(this->data.get())
                            + (this->parent->type->size * i));
            }

            // converts a pointer into this block back into its corresponding
            // index
            inline usize to_index(const void *p) const {
                return static_cast<usize>(
                    (reinterpret_cast<const u8*>(p)
                        - reinterpret_cast<const u8*>(this->data.get()))
                        / this->parent->type->size);
            }
        };

        // constituent component type contained in this array
        ComponentType *type = nullptr;

        Block blocks[NUM_BLOCKS];
        usize n_blocks = 0;

        // implicitly non-copyable
        ComponentArray() = default;
        ComponentArray(ComponentType *type) : type(type) {}

        void resize(usize n) {
            while (n > (this->n_blocks * BLOCK_SIZE)) {
                ASSERT(this->n_blocks != NUM_BLOCKS);
                this->blocks[this->n_blocks] = Block(this, this->n_blocks);
                this->n_blocks++;
            }
        }

        // get block number for element i
        inline u8 block(usize i) const {
            return i / BLOCK_SIZE;
        }

        // component pointer access
        inline BaseComponent *operator[](usize i) const {
            return this->blocks[i / BLOCK_SIZE][i % BLOCK_SIZE];
        }

        // converts a pointer from this component array back into an index
        inline usize to_index(const void *p) const {
            const auto *c = reinterpret_cast<const BaseComponent*>(p);
            return
                usize(c->block * BLOCK_SIZE)
                    + this->blocks[c->block].to_index(p);
        }
    };

    // registry of component types
    using ComponentRegistry = TypeRegistry<T, Component<BaseComponent>>;
    template <typename U>
    using ComponentRegistrar = TypeRegistrar<ComponentRegistry, U>;

    // number of components added
    usize n_components = 0;

    // number of objects (i.e. entities)
    usize size = 0;

    // TODO: consider not using vector, just preallocate for all entities
    std::vector<Bitset128> signatures;
    std::array<ComponentArray, MAX_COMPONENTS> components;
    std::array<ComponentType, MAX_COMPONENTS> component_types;

    // non-copyable
    ComponentManager() { this->resize(256); }
    ComponentManager(const ComponentManager &other) = delete;
    ComponentManager(ComponentManager &&other) = default;
    ComponentManager &operator=(const ComponentManager &other) = delete;
    ComponentManager &operator=(ComponentManager &&other) = default;
    virtual ~ComponentManager() = default;

    // hook called by ComponentRegistrar::init to register component type V
    template <typename V>
    void register_type() {
        const u64 id = this->n_components++;
        ASSERT(this->n_components <= MAX_COMPONENTS);

        Component<V>::_id = id;
        V::_p = static_cast<T*>(this);
        this->component_types[id] = ComponentType::template from<V>(this, id);
        this->components[id] = ComponentArray(&this->component_types[id]);
        this->components[id].resize(this->size);

        LOG(" {}::_id={}", typeid(V).name(), id);

        if constexpr (requires { V::on_register(); }) {
            V::on_register();
        }
    }

    void init() {
        ComponentRegistry::instance().initialize(static_cast<T&>(*this));
    }

    // retrieve object from component pointer
    template <typename C>
    Object object_from_component(const void *c) {
        return Object {
            reinterpret_cast<T*>(this),
            static_cast<I>(
                this->components[C::_id].to_index(c))
        };
    }

    // create a new object, fails if out of space
    std::optional<Object> create(std::optional<u64> preset_id = std::nullopt) {
        const auto id
            = TRY_OPT(preset_id ? *preset_id : this->new_object_id());

        if (id >= this->size) {
            this->resize(math::max<usize>(id, this->size * 2));
        }

        const auto obj =
            Object { reinterpret_cast<T*>(this), static_cast<I>(id) };
        obj.signature().reset();
        this->on_create(obj);
        return obj;
    }

    // destroy an object and all of its components
    void destroy(Object object) {
        this->on_destroy(object);

        auto &signature = object.signature();
        for (usize i = 0; i < MAX_COMPONENTS; i++) {
            if (signature[i]) {
                const auto &type = this->component_types[i];
                auto *base = this->components[type.id][object.id];
                this->on_component_destroy(
                    type.id,
                    reinterpret_cast<void*>(base),
                    object);
                base->~BaseComponent();
            }
        }
        signature.reset();
    }

    // add component to object
    template <typename C>
    inline C &add_component(Object object, C &&component) {
        auto &type = this->component_types[C::_id];
        auto &signature = this->signatures[object];
        ASSERT(
            !signature[type.id],
            "Object {} already has component of type {}",
            object, typeid(C).name());
        signature.set(type.id);

        ASSERT(
            sizeof(C) <= type.size,
            fmt::format(
                "Component {} is too large, size is {} but max size is {}",
                typeid(C).name(), sizeof(C), type.size));

        const auto &arr = this->components[type.id];
        auto *p = arr[object];
        C *c = new (p) C(std::forward<C>(component));
        c->block = arr.block(object);
        c->init();
        this->on_component_create(type.id, p, object);
        return *c;
    }

    //  remove component from an object
    template <typename C>
    inline void remove_component(Object object) {
        const auto &type = this->component_types[C::_id];
        auto *base = this->components[type.id][object];
        C *c = static_cast<C*>(base);
        this->on_component_destroy(
            type.id,
            reinterpret_cast<void*>(c),
            object);
        base->~BaseComponent();
        this->signatures[object].clear(type.id);
    }

    // true if object has component of type C
    template <typename C>
    inline bool has_component(Object object) {
        return this->signatures[object][C::_id];
    }

    // get component C from object, crashes if not present
    template <typename C>
    inline C &component(Object object) {
        const auto id = C::_id;
        ASSERT(
            object.signature()[id],
            fmt::format(
                "{} does not have component {}",
                object.to_string(),
                typeid(C).name()));
        void *p = this->components[id][object];
        return *static_cast<C*>(p);
    }

    // returns optional pointer to object component, nullopt if not present
    // otherwise pointer is valid (does not return nullptr)
    template <typename C>
    inline std::optional<C*> opt_component(Object object) {
        const auto &signature = this->signatures[object];
        const auto id = C::_id;
        auto *c = this->components[id][object];
        return signature[id] ?
            std::optional(static_cast<C*>(c))
            : std::nullopt;
    }

    // check if out of IDs / cannot create more objects
    virtual bool full() const = 0;

    // id generation
    virtual std::optional<u64> new_object_id() = 0;

    // object creation hook
    virtual void on_create(Object object) {}

    // object destruction hook
    virtual void on_destroy(Object object) {}

    // component creation hook
    virtual void on_component_create(u64 id, void *component, Object object) {}

    // component destruction hook
    virtual void on_component_destroy(u64 id, void *component, Object object) {}

private:
    void resize(usize new_size) {
        ASSERT(new_size <= MAX_OBJECTS);
        for (usize i = 0; i < this->n_components; i++) {
            this->components[i].resize(new_size);
        }

        this->signatures.resize(new_size);
        this->size = new_size;
    }
};
