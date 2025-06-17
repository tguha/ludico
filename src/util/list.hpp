#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"
#include "util/allocator.hpp"
#include "util/nameof.hpp"
#include "util/math.hpp"

// list support functions
namespace list {
    // move begin..end -> out
    template <typename It>
    void move(It begin, It end, It out) {
        if (begin < out) {
            out += (end - begin);
            std::move_backward(begin, end, out);
        } else {
            std::move(begin, end, out);
        }
    }

    // copy begin..end -> out
    template <typename It>
    void copy(It begin, It end, It out) {
        if (begin < out) {
            out += (end - begin);
            std::copy_backward(begin, end, out);
        } else {
            std::copy(begin, end, out);
        }
    }
}

// used to control ListImpl::deep_copy memory initialization
namespace ListDeepCopyFlags {
    enum Enum {
        INITIALIZED     = 1 << 0,
        UNINITIALIZED   = 0 << 0
    };
}

// Traits layout:
// struct Traits {
//      static constexpr bool CONTIGUOUS = ...;
// }

// basic list functions for some list _L over a type _T with list traits _Traits
// T must be EITHER move assign/construct-able OR copy assign/construct-able
template <typename _L, typename _T, typename _Traits>
    requires (
        (std::is_move_assignable_v<_T> && std::is_move_constructible_v<_T>)
        || (std::is_copy_assignable_v<_T> && std::is_copy_constructible_v<_T>))
struct ListBase {
    // TODO: some sort of notify_write for COW children

    // underlying list must implement:
    // make an empty list L
    // L create() const
    //
    // pointer to the ith place in the list
    // T *ptr(usize i)
    //
    // pointer to the ith place in the list
    // const T *ptr(usize i) const
    //
    // capacity of underlying memory
    // usize capacity() const
    //
    // reallocate list to have size of at least n elements
    // void realloc(usize n)
    //
    // move elements from "from" to "to"
    // if new space is created, return a pointer to that space
    // if space is removes, elements are written out into "out"
    // void move_elements(usize to, usize from, T *out, usize n_out)
    //
    // * elements are initialized on-demand - unless resize(n) is called,
    //   (*this)[i] is only initialized if i < size

    using L = _L;
    using T = _T;
    using Traits = _Traits;

    using value_type = _T;

private:
    using ThisType = ListBase<L, T, Traits>;
public:

    // iterator
    template <bool CONST = false>
    struct It {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = usize;
        using value_type        = T;
        using pointer           =
            std::conditional_t<CONST, const T*, T*>;
        using reference         =
            std::conditional_t<CONST, const T&, T&>;
        using list_pointer      =
            std::conditional_t<CONST, const ThisType*, ThisType*>;
        using list_reference    =
            std::conditional_t<CONST, const ThisType&, ThisType&>;

        It(list_reference ts, bool end)
            : ts(&ts),
              i(end ? ts.size() : 0),
              p(nullptr) {}

        reference operator*() const {
            if (!this->p) { this->p = &(*this->ts)[this->i]; }
            return *this->p;
        }

        pointer operator->() const {
            if (!this->p) { this->p = &(*this->ts)[this->i]; }
            return this->p;
        }

        It &operator--() { --this->i; this->p = nullptr; return *this; }
        It operator--(int) { auto it = *this; --(*this); return it; }
        It &operator++() { ++this->i; this->p = nullptr; return *this; }
        It operator++(int) { auto it = *this; ++(*this); return it; }

        reference operator[](difference_type j) const {
            return (*ts)[this->i + j];
        }

        bool operator==(const It &rhs) const {
            return this->ts == rhs.ts && this->i == rhs.i;
        }

        bool operator!=(const It &rhs) const {
            return !(*this == rhs);
        }

        auto operator<=>(const It &rhs) const {
            return this->ts == rhs.ts ? this->i <=> rhs.i : this->ts <=> rhs.ts;
        }

       It& operator+=(difference_type off) {
           this->i += off;
           this->p = nullptr;
           return *this;
       }

       It operator+(difference_type off) const {
           return { this->ts, this->i + off };
       }

       friend It operator+(
           difference_type off,
           const It& right) {
           return { right.ts, right.i + off };
       }

       It& operator-=(difference_type off) {
           this->i -= off;
           this->p = nullptr;
           return *this;
       }

       It operator-(difference_type off) const {
           return { this->ts, this->i - off };
       }

       difference_type operator-(const It& right) const {
           return this->i - right.i;
       }

       std::string to_string() const {
           return fmt::format(
               "{}({}, {}, {})",
               NAMEOF_TYPE(decltype(*this)),
               fmt::ptr(this->ts),
               this->i,
               fmt::ptr(this->p));
       }

    private:
        list_pointer ts;
        difference_type i;
        mutable pointer p;

        It(list_pointer ts, difference_type i)
            : ts(ts),
              i(i),
              p(nullptr) {}
    };

    using iterator = It<false>;
    using const_iterator = It<true>;

    // size of list
    inline usize size() const {
        return this->sz;
    }

    // capacity of underlying array
    inline usize capacity() const {
        return this->impl().capacity();
    }

    // returns true if list is empty
    inline bool empty() const {
        return this->sz == 0;
    }

    // iterators
    inline auto begin() { return iterator(*this, false); }
    inline auto begin() const { return const_iterator(*this, false); }
    inline auto cbegin() const { return const_iterator(*this, false); }
    inline auto end() { return iterator(*this, true); }
    inline auto end() const { return const_iterator(*this, true); }
    inline auto cend() const { return const_iterator(*this, true); }

    inline T *data()
            requires Traits::CONTIGUOUS {
        return &(*this)[0];
    }

    inline const T *data() const
            requires Traits::CONTIGUOUS {
        return &(*this)[0];
    }

    inline std::span<T, std::dynamic_extent> span()
            requires Traits::CONTIGUOUS {
        return std::span { &(*this)[0], this->size() };
    }

    inline std::span<const T, std::dynamic_extent> span() const
            requires Traits::CONTIGUOUS {
        return std::span { &(*this)[0], this->size() };
    }

    // span conversion operator
    inline operator std::span<T>() requires Traits::CONTIGUOUS {
        return this->span();
    }

    // span conversion operator
    inline operator std::span<const T>() const requires Traits::CONTIGUOUS {
        return this->span();
    }

    // access element at index i
    inline const T &operator[](usize i) const {
        ASSERT(
            i < this->sz,
            "{} out of range for {} of size {}",
            i, NAMEOF_TYPE(decltype(*this)), i);
        return *this->impl().ptr(i);
    }

    // access element at index i
    inline T &operator[](usize i) {
        ASSERT(
            i < this->sz,
            "{} out of range for {} of size {}",
            i, NAMEOF_TYPE(decltype(*this)), i);
        return *this->impl().ptr(i);
    }

    // reserve space for at least n elements
    // will not shrink the list
    inline void reserve(usize n) {
        if (n > this->capacity()) {
            this->impl().realloc(n, this->sz);
        }
    }

    // resize to n and initialize elements
    inline void resize(usize n)
        requires std::is_default_constructible_v<T> {
        this->impl().realloc(n, this->sz);

        const auto size_old = this->sz;
        this->sz = n;

        if constexpr (NEED_CTOR) {
            for (usize i = size_old; i < this->sz; i++) {
                new (this->impl().ptr(i)) T();
            }
        }
    }

    // resize to n and initialize elements
    inline void resize(usize n, const T &fill) {
        ASSERT_STATIC(COPYABLE, "T is not copyable");

        this->impl().realloc(n, this->sz);

        const auto size_old = this->sz;
        this->sz = n;
        for (usize i = size_old; i < this->sz; i++) {
            new (this->impl().ptr(i)) T(fill);
        }
    }

    // inserts an element at index i
    template <typename U>
    inline T &insert(usize i, U &&u) {
        ASSERT(
            i <= this->sz,
            "{} out of range for ListImpl<{}> of size {}",
            i, NAMEOF_TYPE(T), i);

        const auto at_end = i == this->sz;

        if ((this->sz + 1) >= this->capacity()) {
            this->impl().realloc(this->sz + 1, this->sz);
        }

        const auto size_old = this->sz;
        this->sz++;

        if (!at_end) {
            this->impl().move_elements(i + 1, i, size_old - i);
        }

        T *p = this->impl().ptr(i);
        if (at_end) {
            new (p) T(std::forward<U>(u));
        } else {
            *p = T(std::forward<U>(u));
        }
        return *p;
    }

    // emplaces an element on the end of the list, expanding if necessary
    template <typename ...Args>
    inline T &emplace(Args&& ...args) {
        return this->insert(this->sz, T(std::forward<Args>(args)...));
    }

    // emplaces an element at the specified location, expanding if necessary
    template <typename ...Args>
    inline T &emplace_at(usize i, Args&& ...args) {
        return this->insert(i, T(std::forward<Args>(args)...));
    }

    // pushes an element to end of list, expanding if necessary
    inline T &push(T &&t) { return this->insert(this->sz, std::move(t)); }

    // pushes an element to end of list, expanding if necessary
    inline T &push(const T &t) { return this->insert(this->sz, t); }

    // pushes a list M to this list
    template <typename M, typename U = typename M::value_type>
        requires std::is_rvalue_reference_v<M>
            && std::is_convertible_v<U, T>
    inline void push(M &&ms) {
        this->reserve(this->sz + ms.size());
        for (auto &&m : ms) {
            this->push(std::move(m));
        }
    }

    // pushes a list M to this list
    template <typename M, typename U = typename M::value_type>
        requires std::is_convertible_v<U, T>
    inline void push(const M &ms) {
        this->reserve(this->sz + ms.size());
        for (const auto &m : ms) {
            this->push(m);
        }
    }

    // remove element i from list, contracting if possible
    inline T remove(usize i) {
        T t = this->extract(i);
        if (i != this->sz - 1) {
            this->impl().move_elements(i, i + 1, this->sz - i - 1);
        }
        this->impl().realloc(this->sz - 1, this->sz);
        this->sz--;
        return t;
    }

    // pops an element from the end of the list, contracting if possible
    inline T pop() {
        ASSERT(!this->empty());
        return this->remove(this->size() - 1);
    }

    // clear list, optionally removing reserved capacity
    inline void clear(bool reallocate = true) {
        if (this->sz != 0) {
            // delete existing elements
            if constexpr (NEED_DTOR) {
                for (usize i = 0; i < this->sz; i++) {
                    (*this)[i].~T();
                }
            }

            this->sz = 0;
        }

        if (reallocate) {
            this->impl().realloc(0, 0);
        }
    }

    // extract (either by move or by copy) value at i
    // leaves (*this)[i] in an undefined state
    // moves from (*this)[i] if T is moveable
    inline T extract(usize i) {
        std::aligned_storage_t<sizeof(T), alignof(T)> t;
        T *p = this->impl().ptr(i);

        if constexpr (MOVEABLE) {
            new (&t) T(std::move(*p));
        } else {
            new (&t) T(*p);
        }

        if constexpr (MOVEABLE) {
            return std::move(*std::launder(reinterpret_cast<T*>(&t)));
        } else {
            return *std::launder(reinterpret_cast<T*>(&t));
        }
    }

    // deep_copy this list with some copy operator
    // if FLAGS & INITIALIZED, then the second argument to f_copy (T*) will be
    // initialized memory (by default ctor)
    template <typename F>
        requires (requires (F f, const T *a, T *b) { { f(a, b) }; })
    inline L deep_copy(
        F &&f_copy,
        ListDeepCopyFlags::Enum flags = ListDeepCopyFlags::INITIALIZED) const {
        L l = this->impl().create();

        if (flags & ListDeepCopyFlags::INITIALIZED) {
            l.resize(this->size());
        } else {
            l.realloc(this->size(), 0);
            l.sz = this->size();
        }

        for (usize i = 0; i < this->size(); i++) {
            f_copy(this->ptr(i), l.ptr(i));
        }
    }

protected:
    static constexpr auto
        HAS_CTOR  = std::is_default_constructible_v<T>,
        NEED_CTOR = !std::is_trivially_default_constructible_v<T>,
        NEED_DTOR = !std::is_trivially_move_assignable_v<T>,
        NEED_COPY = !std::is_trivially_copyable_v<T>,
        NEED_MOVE =
            !(std::is_trivially_move_constructible_v<T>
                || std::is_trivially_move_assignable_v<T>),
        MOVEABLE = std::is_move_constructible_v<T>,
        COPYABLE = std::is_copy_constructible_v<T>,
        CAN_MEMCPY =
            (MOVEABLE && !NEED_MOVE) || (COPYABLE && !NEED_COPY);

    // size of list
    usize sz = 0;

private:
    const L &impl() const { return *static_cast<const L*>(this); }
    L &impl() { return *static_cast<L*>(this); }
};

// struct for list settings
// expects:
// double factor (list grow/shrink factor)
// usize default_size (default size of list on allocation)
// usize multiple (if list should be grown by multiples of N elements)
struct ListConfigDefault {
    static constexpr auto factor = 1.618;
    static constexpr auto default_size = 8;
    static constexpr auto multiple = 0;
};

struct ListTraits {
    static constexpr bool CONTIGUOUS = true;
};

// "classic" list which expands and contracts while moving all of its elements
// such that they are always contiguous in memory. no guarantees of poiner
// stability.
template <typename T, typename R = ListConfigDefault>
    requires (
        ((requires { { R::factor } -> std::convertible_to<f32>; })
        || (requires { { R::multiple } -> std::convertible_to<usize>; }))
        && (requires { { R::default_size } -> std::convertible_to<usize>; }))
struct List : public ListBase<List<T, R>, T, ListTraits> {
    using Base           = ListBase<List<T, R>, T, ListTraits>;
    using value_type     = typename Base::value_type;
    using iterator       = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    Allocator *allocator = nullptr;

    List() = default;
    List(const List &other) { *this = other; }
    List(List &&other) { *this = std::move(other); }

    List &operator=(const List &other) {
        ASSERT(
            other.allocator,
            "copy from uninitialized {} {} -> {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(&other),
            fmt::ptr(this));

        Base::operator=(other);

        // TODO: copy-on-write
        this->allocator = other.allocator;
        this->cap = other.cap;

        if (this->cap != 0) {
            this->ts =
                this->allocator->template alloc_array<T>(
                    this->cap);

            if constexpr (Base::CAN_MEMCPY) {
                std::memcpy(
                    this->ts,
                    other.ts,
                    this->sz * sizeof(T));
            } else {
                for (usize i = 0; i < this->sz; i++) {
                    new (&this->ts[i]) T(other.ts[i]);
                }
            }
        } else {
            this->ts = nullptr;
        }

        return *this;
    }

    List &operator=(List &&other) {
        Base::operator=(std::move(other));

        this->allocator = other.allocator;
        this->cap = other.cap;
        this->ts = other.ts;

        other.allocator = nullptr;
        other.cap = 0;
        other.ts = nullptr;
        return *this;
    }

    ~List() {
        if (this->ts) {
            ASSERT(
                this->allocator,
                "{} @ {} does not have an allocator",
                NAMEOF_TYPE(decltype(*this)),
                fmt::ptr(this));

            // dtors for ts are handled by allocator
            this->allocator->free(this->ts);
            this->ts = nullptr;
            this->cap = 0;
        }
    }

    explicit List(Allocator *allocator, usize size = 0)
        : Base(),
          allocator(allocator) {
        if (size > 0) {
            this->resize(size);
        }
    }

private:
    friend Base;

    // extract type traits for different expansion patterns
    static constexpr auto
        HAS_MULTIPLE = (requires { R::multiple; }) && R::multiple != 0,
        HAS_FACTOR = (requires { R::factor; });

    static_assert(
        !HAS_MULTIPLE || ((R::default_size % R::multiple) == 0),
        "default_size % multiple != 0");

    T *ts = nullptr;
    usize cap = 0;

    List<T, R> create() const {
        return List(this->allocator);
    }

    T *ptr(usize i) { return &this->ts[i]; }

    const T *ptr(usize i) const { return &this->ts[i]; }

    usize capacity() const { return this->cap; }

    void realloc(usize n, usize size) {
        if (n == this->cap) {
            return;
        }

        ASSERT(
            this->allocator,
            "{} @ {} does not have an allocator",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        T *old = this->ts;
        if (old) {
            ASSERT(this->cap > 0);
        }

        if constexpr (HAS_FACTOR) {
            const auto
                expand = n > this->cap,
                contract = n <= static_cast<usize>(this->cap / R::factor);
            if (expand) {
                // pick largest of: regular expand, requested size, default size
                this->cap =
                    math::max<usize>(
                        this->cap * R::factor,
                        math::max<usize>(
                            n,
                            R::default_size));
            } else if (contract) {
                this->cap =
                    n == 0 ?
                        0
                        : static_cast<usize>(this->cap / R::factor);
            } else {
                return;
            }
        } else {
            const auto
                expand = n > this->cap,
                contract = n <= static_cast<usize>(this->cap - R::multiple);
            const auto least_multiple =
                (n % R::multiple) == 0 ?
                    n
                    : ((n / R::multiple) + 1) * R::multiple;
            if (expand) {
               this->cap = math::max<usize>(least_multiple, R::default_size);
            } else if (contract) {
                // allow contraction to zero
                this->cap = n == 0 ? 0 : least_multiple;
            } else {
                return;
            }
        }

        if (this->cap == 0) {
            this->ts = nullptr;
        } else {
            this->ts = this->allocator->template alloc_array<T>(this->cap);
        }

        if (old && this->ts) {
            if constexpr (Base::CAN_MEMCPY) {
                std::memcpy(
                    this->ts,
                    old,
                    math::min<usize>(n, size) * sizeof(T));
            } else {
                for (usize i = 0; i < math::min<usize>(n, size); i++) {
                    if constexpr (Base::MOVEABLE) {
                        new (&this->ts[i]) T(std::move(old[i]));
                    } else {
                        new (&this->ts[i]) T(old[i]);
                    }
                }
            }
        }

        if (old) {
            this->allocator->free(old);
        }
    }

    void move_elements(usize to, usize from, usize n) {
        if constexpr (Base::MOVEABLE) {
            list::move(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        } else {
            list::copy(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        }
    }
};

struct FixedListTraits {
    static constexpr bool CONTIGUOUS = true;
};

// list which does not heap allocate with a backing capacity of a fixed size N
// will ASSERT fail if attempts are made to expand over N
template <typename T, usize N>
    requires (N != 0)
struct FixedList : public ListBase<FixedList<T, N>, T, FixedListTraits> {
    using Base           = ListBase<FixedList<T, N>, T, FixedListTraits>;
    using value_type     = typename Base::value_type;
    using iterator       = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    FixedList(usize size = 0) : Base() {
        this->resize(size);
    }

    FixedList(const FixedList&) = default;
    FixedList(FixedList&&) = default;
    FixedList &operator=(const FixedList&) = default;
    FixedList &operator=(FixedList&&) = default;

    // we do not need to explicitly destruct elements as their array is a field
    // (not a pointer!) directly on this fixed list
    ~FixedList() = default;

private:
    friend Base;

    FixedList<T, N> create() const { return FixedList<T, N>(); }

    T *ptr(usize i) { return &this->ts[i]; }
    const T *ptr(usize i) const { return &this->ts[i]; }
    usize capacity() const { return N; }

    void realloc(usize n, usize size) {
        ASSERT(
            n <= N,
            "{} @ {} cannot realloc to capacity {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this),
            n);
    }

    void move_elements(usize to, usize from, usize n) {
        if constexpr (Base::MOVEABLE) {
            list::move(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        } else {
            list::copy(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        }
    }

    T ts[N] = {};
};

struct BlockListTraits {
    static constexpr bool CONTIGUOUS = false;
};

// list which makes no guarantees of contigiuous memory BUT maintains pointer
// locations throughout expansions as it allocates in blocks with the underlying
// allocator
template <typename T, usize N>
    requires (N != 0)
struct BlockList : public ListBase<BlockList<T, N>, T, BlockListTraits> {
    using Base           = ListBase<BlockList<T, N>, T, BlockListTraits>;
    using value_type     = typename Base::value_type;
    using iterator       = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    // backing allocator
    Allocator *allocator = nullptr;

    BlockList() = default;
    BlockList(const BlockList &other) { *this = other; }
    BlockList(BlockList &&other) { *this = std::move(other); }

    BlockList &operator=(const BlockList &other) {
        ASSERT(
            other.allocator,
            "copy from uninitialized {} {} -> {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(&other),
            fmt::ptr(this));

        Base::operator=(other);

        // TODO: copy-on-write
        this->allocator = other.allocator;
        this->lists = decltype(this->lists)(this->allocator);
        this->lists.resize(other.lists.size());
        for (usize i = 0; i < this->lists.size(); i++) {
            this->lists[i] =
                this->allocator->template alloc<FixedList<T, N>>(
                    other.lists[i]);
        }
        return *this;
    }

    BlockList &operator=(BlockList &&other) {
        Base::operator=(std::move(other));

        this->allocator = other.allocator;
        this->lists = std::move(other.lists);
        other.allocator = nullptr;
        return *this;
    }

    ~BlockList() {
        if (!this->allocator) {
            return;
        }

        for (usize i = 0; i < this->lists.size(); i++) {
            this->allocator->free(this->lists[i]);
        }

        this->lists.clear();
        this->allocator = nullptr;
    }

    explicit BlockList(Allocator *allocator, usize size = 0)
        : Base(),
          allocator(allocator),
          lists(allocator) {
        if (size > 0) {
            this->resize(size);
        }
    }

private:
    friend Base;

    // internal lists
    // TODO: list expansion policy should be one at a time
    List<FixedList<T, N>*> lists;

    BlockList<T, N> create() const {
        return BlockList<T, N>(this->allocator);
    }

    T *ptr(usize i) { return &(*this->lists[i / N])[i % N]; }

    const T *ptr(usize i) const { return &(*this->lists[i / N])[i % N]; }

    usize capacity() const { return N * this->lists.size(); }

    void realloc(usize n, usize size) {
        ASSERT(
            this->allocator,
            "{} @ {} does not have an allocator",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        if (n == 0) {
            this->lists.clear();
            return;
        }

        const auto n_lists = (n / N) + 1;

        while (n_lists < this->lists.size()) {
            this->allocator->free(this->lists.pop());
        }

        while (n_lists > this->lists.size()) {
            // resize by default to size N so ptr access works
            this->lists.emplace(
                this->allocator->template alloc<FixedList<T, N>>(N));
        }
    }

    // TODO: optimize
    void move_elements(usize to, usize from, usize n) {
        if constexpr (Base::MOVEABLE) {
            list::move(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        } else {
            list::copy(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        }
    }
};

struct SmallListTraits {
    static constexpr bool CONTIGUOUS = false;
};

// TODO: have contiguous/non-contiguous SmallList variants
// list which stores a small number of elements inline, using a List<T, R> to
// store all elements past N
template <typename T, usize N, typename R = ListConfigDefault>
struct SmallList : public ListBase<SmallList<T, N, R>, T, SmallListTraits> {
    using Base           = ListBase<SmallList<T, N, R>, T, SmallListTraits>;
    using value_type     = typename Base::value_type;
    using iterator       = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    Allocator *allocator;

    SmallList() = default;

    explicit SmallList(Allocator *allocator, usize size = 0)
        : Base(),
          allocator(allocator),
          us(allocator) {
        if (size > 0) {
            this->resize(size);
        }
    }

private:
    friend Base;

    List<T, R> us;
    T ts[N] = {};

    SmallList create() const {
        return SmallList(this->allocator);
    }

    T *ptr(usize i) {
        return i < N ? &this->ts[i] : &this->us[i - N];
    }

    const T *ptr(usize i) const {
        return i < N ? &this->ts[i] : &this->us[i - N];
    }

    usize capacity() const { return N + this->us.size(); }

    void realloc(usize n, usize size) {
        ASSERT(
            this->allocator,
            "{} @ {} does not have an allocator",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        if (n <= N) {
            // can fit inside of inline list
            if (this->us.size() > 0) {
                this->us.clear();
            }
        } else {
            // ensure extra capacity in us
            this->us.resize(n - N);
        }
    }

    // TODO: optimize
    void move_elements(usize to, usize from, usize n) {
        if constexpr (Base::MOVEABLE) {
            list::move(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        } else {
            list::copy(
                this->begin() + from,
                this->begin() + from + n,
                this->begin() + to);
        }
    }
};
