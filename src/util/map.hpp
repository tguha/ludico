#pragma once

#include "util/allocator.hpp"
#include "util/list.hpp"
#include "util/collections.hpp"

struct MapConfigDefault {
    static constexpr f64 LOAD_HIGH = 0.75, LOAD_LOW = 0.10;
};

template <typename T>
concept MapEntry = requires {
    typename T::KeyType;
    typename T::ValueType;
    typename T::ENTRY_TYPE_IDENTIFIER;
};

// flat hash map type for a default constructible K and V
template <
    typename K,
    typename V,
    typename R = MapConfigDefault,
    typename KeyHash = std::hash<K>>
    requires (
        std::is_default_constructible_v<K>
        && std::is_default_constructible_v<V>)
struct Map {
    struct Entry;
private:
    // distance is stored as a 5-bit unsigned integer, so 31 is max value
    static constexpr auto DISTANCE_MAX = 31;

    // bits to shift hash right before we have top 10 (Entry::hash) bits of it
    static constexpr auto HASH_SHIFT_SIZE = 54;

    // planetmath.org/goodhashtableprimes
    static constexpr auto PRIMES =
        types::make_array(
            11, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157,
            98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917,
            25165843, 50331653, 100663319, 201326611, 402653189, 805306457,
            1610612741);

    // internal hash type
    using hash_type = u64;

    // type of this Map<...>
    using ThisType = Map<K, V, R, KeyHash>;

    // for iterators
    struct PtrToE { auto *operator()(auto &e) const { return &e; } };
    struct PtrToK { auto *operator()(auto &e) const { return &e.key; } };
    struct PtrToV { auto *operator()(auto &e) const { return &e.value; } };

public:
    // map entry
    struct Entry {
        // used for structured bindings
        using ENTRY_TYPE_IDENTIFIER = void;

        using KeyType = K;
        using ValueType = V;

        K key;
        V value;

        Entry() : used(false) {}

        std::string to_string() const
            requires (
                fmt::is_formattable<K>::value
                && fmt::is_formattable<V>::value) {
            return fmt::format("{{{}, {}}}", this->key, this->value);
        }

        template <std::size_t I>
        auto &&get() & { return this->get<I>(*this); };

        template <std::size_t I>
        auto &&get() const& { return this->get<I>(*this); };

        template <std::size_t I>
        auto &&get() && { return this->get<I>(*this); };

        template <std::size_t I>
        auto &&get() const&& { return this->get<I>(*this); };

    private:
        friend struct Map;

        bool used      : 1;
        u8 dist        : 5;
        hash_type hash : 10;

        template <typename X, typename Y>
        Entry(X &&x, Y &&y, bool used, u8 dist, hash_type hash)
            : key(std::forward<X>(x)),
              value(std::forward<Y>(y)),
              used(used),
              dist(dist),
              hash(hash) {}

        template<std::size_t I, typename T>
        auto &&get(T &&t) {
            static_assert(I < 2);
            if constexpr (I == 0) return std::forward<T>(t).key;
            if constexpr (I == 1) return std::forward<T>(t).value;
        }

        template<std::size_t I, typename T>
        auto &&get(T &&t) const {
            static_assert(I < 2);
            if constexpr (I == 0) return std::forward<T>(t).key;
            if constexpr (I == 1) return std::forward<T>(t).value;
        }
    };

    // value_type for stdlib friendliness
    using value_type = Entry;

    // map iterator over some type X (either Entry, K, V) and a way to get X*-s
    // from Entry&s (PtrFromEntry)
    template <typename X, typename PtrFromEntry, bool CONST = false>
    struct It {
        using iterator_categoy = std::forward_iterator_tag;
        using difference_type  = usize;
        using value_type       = X;
        using pointer          =
            std::conditional_t<CONST, const value_type*, value_type*>;
        using reference        =
            std::conditional_t<CONST, const value_type&, value_type&>;
        using map_pointer      =
            std::conditional_t<CONST, const ThisType*, ThisType*>;
        using map_reference    =
            std::conditional_t<CONST, const ThisType&, ThisType&>;

        It(map_reference m, bool end)
            : m(&m),
              i(end ? m.entries.size() : 0),
              p(nullptr) {
              if (!end) {
                  this->next(true);
              }
          }

        // implicit conversion to const iterator
        operator It<X, PtrFromEntry, true>() const {
            return this->as_const();
        }

        reference operator*() const {
            if (!this->p) {
                ASSERT(
                    i < this->m->entries.size(),
                    "{} @ {} out of range {}/{}",
                    NAMEOF_TYPE(decltype(*this)),
                    fmt::ptr(this),
                    i,
                    this->m->entries.size());
                this->p = PtrFromEntry()(this->m->entries[this->i]);
            }
            return *this->p;
        }

        pointer operator->() const {
            return &**this;
        }

        It &operator++() { this->next(); this->p = nullptr; return *this; }
        It operator++(int) { auto it = *this; ++(*this); return it; }

        bool operator==(const It &rhs) const {
            return this->m == rhs.m && this->i == rhs.i;
        }

        bool operator!=(const It &rhs) const {
            return !(*this == rhs);
        }

        auto operator<=>(const It &rhs) const {
            return this->m == rhs.m ? this->i <=> rhs.i : this->m <=> rhs.m;
        }

        std::string to_string() const {
            return fmt::format(
                "{}({}, {}, {})",
                NAMEOF_TYPE(decltype(*this)),
                fmt::ptr(this->m),
                this->i,
                fmt::ptr(this->p));
        }

    private:
        friend struct Map;

        It<X, PtrFromEntry, true> as_const() const {
            return It<X, PtrFromEntry, true>(*this->m, this->i, this->p);
        }

        It<X, PtrFromEntry, false> unconst() const {
            return It<X, PtrFromEntry, false>(
                const_cast<ThisType&>(
                    *this->m),
                this->i,
                const_cast<value_type*>(
                    this->p));
        }

        void next(bool init = false) {
            if (!init) {
                this->i++;
            }

            while (
                this->i != this->m->entries.size()
                && !this->m->entries[this->i].used) {
                this->i++;
            }
        }

        It(map_reference m, usize i, pointer p = nullptr)
            : m(&m),
              i(i),
              p(p) {}

        map_pointer m;
        usize i;
        mutable pointer p;
    };

    // entry iterators
    using iterator = It<Entry, PtrToE, false>;
    using const_iterator = It<Entry, PtrToE, true>;

    // key iterators
    using key_iterator = It<K, PtrToK, false>;
    using const_key_iterator = It<K, PtrToK, true>;

    // value iterators
    using value_iterator = It<K, PtrToV, false>;
    using const_value_iterator = It<K, PtrToV, true>;

private:
    static hash_type hash_key(const K &k) {
        return KeyHash()(k);
    }

    // try to resize the map, optionally "forcing" a resize
    // returns true on rehash
    bool try_rehash(bool force = false) {
        const auto load = this->used / static_cast<f64>(this->capacity());
        const auto
            low = load < R::LOAD_LOW,
            high = load > R::LOAD_HIGH;

        if (!force && (
                (!low && !high)
                || (low && this->prime_index == 0)
                || (high && (this->prime_index == PRIMES.size() - 1)))) {
            // don't force if not necessary
            return false;
        }

        // move up or down to the next prime OR do nothing
        // (force and not high/low)
        if (low && this->prime_index != 0) {
            this->prime_index--;
        } else if (force || (high && this->prime_index != PRIMES.size() - 1)) {
            this->prime_index++;
        }

        ASSERT(
            this->prime_index < PRIMES.size(),
            "{} @ {} is out of space",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        auto old_entries = std::move(this->entries);

        this->entries = List<Entry>(old_entries.allocator);
        this->entries.resize(PRIMES[this->prime_index]);

        // re-insert all entries
        for (usize i = 0; i < old_entries.size(); i++) {
            auto &e = old_entries[i];
            if (e.used) {
                this->insert(
                    Map::hash_key(e.key),
                    std::move(e.key),
                    std::move(e.value),
                    false);
            }
        }

        return true;
    }

    // flags for insert(...)
    enum InsertFlags : u8 {
        ENTRY_IS_NEW = (1 << 0),
        REPLACE_IF_EXISTS = (1 << 1)
    };

    // inserts a new element into the map, returns its inserted position
    template <typename X, typename Y>
    usize insert(
        hash_type hash,
        X &&key,
        Y &&value,
        u8 flags) {
        // check if fully empty - if so, allocate to bare minimum
        if (this->entries.size() == 0) {
            this->entries.resize(PRIMES[this->prime_index]);
        }

        usize pos = hash % this->capacity();
        hash_type hashbits = hash >> HASH_SHIFT_SIZE;
        usize dist = 0;

        while (true) {
            Entry &entry = this->entries[pos];
            if (!entry.used || entry.dist < dist) {
                Entry new_entry(
                    std::forward<X>(key),
                    std::forward<Y>(value),
                    true,
                    dist,
                    hashbits);

                if (flags & ENTRY_IS_NEW) {
                    this->used++;
                    ASSERT(
                        this->capacity() != PRIMES[PRIMES.size() - 1]
                        || this->used != this->capacity(),
                        "{} @ {} ran out of space",
                        NAMEOF_TYPE(decltype(*this)),
                        fmt::ptr(this));
                }

                if (entry.used) {
                    // this entry's distance is less than that of the one we're
                    // trying to insert, so displace this element to reduce long
                    // lookup times
                    Entry old = std::move(entry);
                    entry = std::move(new_entry);
                    this->insert(
                        Map::hash_key(old.key),
                        std::move(old.key),
                        std::move(old.value),
                        flags & ~ENTRY_IS_NEW);
                    break;
                } else {
                    // copy directly
                    entry = std::move(new_entry);
                }

                break;
            } else if (
                entry.dist == dist
                && entry.hash == hashbits
                && key == entry.key) {
                if (flags & REPLACE_IF_EXISTS) {
                    // entry already exists, just replace value
                    entry.value = std::move(value);
                }
                break;
            }

            // rehash if we have a distance greater than that which the storage
            // allows
            dist++;
            if (dist >= DISTANCE_MAX) {
                this->try_rehash(true);
                return
                    this->insert(
                        hash,
                        std::move(key),
                        std::move(value),
                        flags | ENTRY_IS_NEW);
            }

            // check the next location
            pos = (pos + 1) % (this->capacity());
        }

        // rehash if necessary
        if (this->try_rehash()) {
            return this->find(hash, key);
        }

        return pos;
    }

    // find index of entry corresponding to key, -1 if not found
    usize find(hash_type hash, const K &key) const {
        if (this->capacity() == 0) {
            return -1;
        }

        usize pos = hash % this->capacity(), count = 0;
        hash_type hashbits = hash >> HASH_SHIFT_SIZE;

        while (true) {
            const Entry &e = this->entries[pos];

            // empty entry? does not exist
            if (!e.used) {
                return -1;
            }

            if (e.hash == hashbits
                && e.key == key) {
                return pos;
            }

            count++;

            // looked through whole list and found nothing
            if (count == this->entries.size()) {
                return -1;
            }

            // advance position, looping if necessary
            pos = (pos + 1) % this->capacity();
        }

        // TODO: should not to be reached?
        return -1;
    }

    // remove an entry at a specified position
    Entry remove_at(usize pos) {
        ASSERT(
            pos != static_cast<usize>(-1),
            "{} @ {} cannot remove at invalid position",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        usize next_pos = 0;

        this->used--;

        Entry *next = nullptr, *current = &this->entries[pos];

        ASSERT(
            current->used,
            "{} @ {} cannot remove at position {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this),
            pos);

        // remove current entry
        Entry result = std::move(*current);
        *current = Entry();

        // shift further entries back by one
        while (true) {
            next_pos = (pos + 1) % this->capacity();
            next = &this->entries[next_pos];

            // stop when we find a 0 distance entry (must have this/another key)
            // or an empty entry
            if (!next->used || next->dist == 0) {
                break;
            }

            // move next entry into this one
            *current = std::move(*next);
            *next = Entry();

            // advance
            pos = next_pos;
        }

        // check if we need to rehash
        this->try_rehash();

        return result;
    }

    // remove entry with specified key
    Entry remove(hash_type hash, const K &key) {
        const auto pos = this->find(hash, key);
        ASSERT(
            pos != static_cast<usize>(-1),
            "{} @ {} cannot remove key with hash {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this),
            hash);
        return this->remove_at(pos);
    }

    // capacity of backing storage
    usize capacity() const {
        return this->entries.size();
    }

    // index of current prime into PRIMES
    usize prime_index = 0;

    // number of used entries
    usize used = 0;

    // internal entry storage
    List<Entry> entries;
public:
    Allocator *allocator = nullptr;

    Map() = default;

    explicit Map(Allocator *allocator)
        : entries(allocator),
          allocator(allocator) {}

    Map(Allocator *allocator,
        std::initializer_list<std::tuple<K, V>> &&pairs)
        : Map(allocator) {
        for (auto &&[k, v] : std::move(pairs)) {
            this->insert(std::move(k), std::move(v));
        }
    }

    Map(const Map&) = default;
    Map &operator=(const Map&) = default;

    Map(Map &&other) { *this = std::move(other); }

    Map &operator=(Map &&other) {
        this->prime_index = other.prime_index;
        this->used = other.used;
        this->entries = std::move(other.entries);
        this->allocator = other.allocator;

        other.used = 0;
        other.prime_index = 0;
        other.allocator = nullptr;
        return *this;
    }

    // iterators
    auto begin() { return iterator(*this, false); }
    auto begin() const { return const_iterator(*this, false); }
    auto cbegin() const { return const_iterator(*this, false); }
    auto end() { return iterator(*this, true); }
    auto end() const { return const_iterator(*this, true); }
    auto cend() const { return const_iterator(*this, true); }

    auto begin_keys() { return key_iterator(*this, false); }
    auto begin_keys() const { return const_key_iterator(*this, false); }
    auto end_keys() { return key_iterator(*this, true); }
    auto end_keys() const { return const_key_iterator(*this, true); }

    auto begin_values() { return value_iterator(*this, false); }
    auto begin_values() const { return const_value_iterator(*this, false); }
    auto end_values() { return value_iterator(*this, true); }
    auto end_values() const { return const_value_iterator(*this, true); }

    auto pairs() const {
        return collections::iterable<const_iterator>(
            this->begin(),
            this->end());
    }

    auto pairs() {
        return collections::iterable<iterator>(
            this->begin(),
            this->end());
    }

    auto keys() const {
        return collections::iterable<const_key_iterator>(
            this->begin_keys(),
            this->end_keys());
    }

    auto keys() {
        return collections::iterable<key_iterator>(
            this->begin_keys(),
            this->end_keys());
    }

    auto values() const {
        return collections::iterable<const_value_iterator>(
            this->begin_values(),
            this->end_values());
    }

    auto values() {
        return collections::iterable<value_iterator>(
            this->begin_values(),
            this->end_values());
    }

    // size of this map
    usize size() const {
        return this->used;
    }

    // true if this map is empty
    bool empty() const {
        return this->size() == 0;
    }

    // clears the map
    void clear() {
        this->prime_index = 0;
        this->used = 0;
        this->entries.clear();
    }

    // returns true if map contains key k
    bool contains(const K &k) const {
        return this->find(Map::hash_key(k), k) != static_cast<usize>(-1);
    }

    // find iterator to key k
    const_iterator find(const K &k) const {
        const auto pos = this->find(Map::hash_key(k), k);

        if (pos == static_cast<usize>(-1)) {
            return this->end();
        }

        return const_iterator(*this, pos);
    }

    // find iterator to key k
    iterator find(const K &k) {
        return const_cast<const Map*>(this)->find(k).unconst();
    }

    // tries to insert an element at k, does nothing if it already exists.
    // returns iterator to inserted element if no element existed, otherwise
    // returns iterator to existing element.
    template <typename U>
    iterator try_insert(const K &k, U &&u) {
        const auto pos =
            this->insert(
                Map::hash_key(k),
                k,
                std::forward<U>(u),
                ENTRY_IS_NEW);
        return iterator(*this, pos);
    }

    // inserts an element at k, overwriting the existing element if it already
    // exists. returns an iterator to the new element
    template <typename U>
    iterator insert(const K &k, U &&u) {
        const auto pos =
            this->insert(
                Map::hash_key(k),
                k,
                std::forward<U>(u),
                ENTRY_IS_NEW | REPLACE_IF_EXISTS);
        return iterator(*this, pos);
    }

    // removes the element at the specified iterator
    Entry remove(const const_iterator &it) {
        return this->remove_at(it.i);
    }

    // removes the element at the specified iterator
    Entry remove(const iterator &it) {
        return this->remove_at(it.i);
    }

    // remove element at the specified key if it exists
    std::optional<Entry> remove(const K &k) {
        auto it = this->find(k);
        return it == this->end() ?
            std::nullopt
            : std::make_optional<Entry>(this->remove(it));
    }

    inline const V &operator[](const K &k) const {
        const auto pos = this->find(Map::hash_key(k), k);
        ASSERT(
            pos != static_cast<usize>(-1),
            "{} @ {}: no such key {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this),
            k);
        return this->entries[pos].value;
    }

    inline V &operator[](const K &k) {
        return const_cast<V&>(const_cast<const Map*>(this)->operator[](k));
    }

    inline const V &operator[](const const_iterator &it) const {
        Entry *e = &*it;
        ASSERT(e->used);
        return &e->value;
    }

    inline V &operator[](const const_iterator &it) {
        return const_cast<V&>(this->operator[](it));
    }

    inline const V &operator[](const iterator &it) const {
        return this->operator[](it.as_const());
    }

    inline V &operator[](const iterator &it) {
        return this->operator[](it.as_const());
    }

    inline bool operator==(const Map &rhs) const {
        if (this->size() != rhs.size()) {
            return false;
        }

        for (const auto &[k, v] : *this) {
            auto it = rhs.find(k);
            if (it == rhs.end() || it->value != v) { return false; }
        }

        for (const auto &[k, v] : rhs) {
            auto it = this->find(k);
            if (it == this->end() || it->value != v) { return false; }
        }

        return true;
    }

    inline bool operator!=(const Map &rhs) const {
        return !(*this == rhs);
    }

    // merge another map into this map
    template <typename M>
    void merge(M &&ms) {
        if constexpr (std::is_rvalue_reference_v<M>) {
            for (usize i = 0; i < ms.entries.size(); i++) {
                Entry &e = ms.entries[i];
                if (e.used) {
                    this->insert(std::move(e.key), std::move(e.value));
                }
            }
            ms.clear();
        } else {
            for (const auto &[k, v] : ms) {
                this->insert(k, v);
            }
        }
    }
};

// structured bindings for Map::Entry
namespace std {
    template <MapEntry E>
    struct tuple_size<E> : public std::integral_constant<size_t, 2> {};

    template <MapEntry E>
    struct tuple_element<0, E> {
        using type = typename E::KeyType;
    };

    template <MapEntry E>
    struct tuple_element<1, E> {
        using type = typename E::ValueType;
    };

    template <MapEntry E>
    struct tuple_element<0, const E> {
        using type = const typename E::KeyType;
    };

    template <MapEntry E>
    struct tuple_element<1, const E> {
        using type = const typename E::ValueType;
    };
} // end namespace std
