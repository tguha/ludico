#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"
#include "util/hash.hpp"

namespace collections {
// generic iterable
// NOTE: implicitly assumes that iterators can be reused
template <typename It>
struct iterable {
    using value_type = typename It::value_type;

    iterable(It &&begin, It &&end)
        : it_begin(begin),
          it_end(end) {}

    It begin() const { return this->it_begin; }
    It end() const { return this->it_end; }

    typename It::reference nth(usize n) const {
        if constexpr (std::random_access_iterator<It>) {
            return *(this->it_begin + n);
        }

        ASSERT(n < this->count());
        usize i = 0;
        for (typename It::reference el : *this) {
            if (i == n) {
                return el;
            }
            i++;
        }

        std::abort();
    }

    std::iter_difference_t<It> count() const {
        if constexpr (std::random_access_iterator<It>) {
            return this->it_end - this->it_begin;
        }

        usize i = 0;
        for (UNUSED auto &_ : *this) {
            i++;
        }
        return i;
    }

private:
    It it_begin, it_end;
};

// NOTE: add as needed!
// generic container append

// vectors, dequeues
template <typename C, typename V>
    requires types::is_vector<C>::value || types::is_deque<C>::value
auto append(C &c, V &&v) {
    c.emplace_back(std::forward<V>(v));
}

// sets
template <typename C, typename V>
    requires types::is_set<C>::value || types::is_unordered_set<C>::value
auto append(C &c, V &&v) {
    c.insert(std::forward<V>(v));
}

// iterator backed by generic collections::append
template <typename C>
struct append_iterator {
    static constexpr auto has_append =
        (requires (C c, typename C::value_type v) {
            collections::append(c, std::move(v));
        });

    static constexpr auto is_array_like =
        types::is_array<C>::value
        || types::is_span<C>::value;

    static_assert(
        has_append || is_array_like,
        "type has neither append nor is array-like");

    using V = typename C::value_type;

    // for LegacyOutputIterator
    using container_type = C;
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    explicit append_iterator(C &c) : container(&c) {}

    template <typename U>
    append_iterator<C> &operator=(U &&u) {
        if constexpr (has_append) {
            collections::append(
                *this->container, std::forward<U>(u));
        } else {
            (*this->container)[i] = std::forward<U>(u);
        }
        i++;
        return *this;
    }

    append_iterator<C> &operator*() {
        return *this;
    }

    append_iterator<C> &operator++() {
        return *this;
    }

    append_iterator<C> &operator++(int) {
        return *this;
    }

private:
    C *container;
    usize i = 0;
};

template <typename InputIt, typename OutputIt, typename F>
OutputIt move(InputIt first, InputIt last, OutputIt d_first, F &&conv) {
    while (first != last) {
        *d_first++ = std::move(conv(std::move(*first++)));
    }
    return d_first;
}

template <typename InputIt, typename OutputIt>
OutputIt move(InputIt first, InputIt last, OutputIt d_first) {
    while (first != last) {
        *d_first++ = std::move(*first++);
    }
    return d_first;
}

template <typename InputIt, typename OutputIt, typename F>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first, F &&conv) {
    while (first != last) {
        *d_first++ = conv(*first++);
    }
    return d_first;
}

template <typename InputIt, typename OutputIt>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first) {
    while (first != last) {
        *d_first++ = *first++;
    }
    return d_first;
}

template <typename R, typename T>
R move_to(T &&ts) {
    auto res = R();
    std::move(ts.begin(), ts.end(), append_iterator(res));
    return res;
}

template <typename R, typename T, typename F>
R move_to(T &&ts, F &&conv) {
    auto res = R();
    collections::move(
        ts.begin(),
        ts.end(),
        append_iterator(res),
        std::forward<F>(conv));
    return res;
}

template <typename R, typename T>
R copy_to(const T &ts, R &rs) {
    collections::copy(
        ts.begin(),
        ts.end(),
        append_iterator(rs));
    return rs;
}

template <typename R, typename T>
R copy_to(const T &ts) {
    R rs;
    collections::copy_to(ts, rs);
    return rs;
}

template <typename R, typename T, typename F>
R copy_to(const T &ts, F &&conv) {
    R rs;
    collections::copy(
        ts.begin(),
        ts.end(),
        append_iterator(rs),
        std::forward<F>(conv));
    return rs;
}

template <typename R, typename T, typename F>
R &copy_to(const T &ts, R &rs, F &&conv) {
    collections::copy(
        ts.begin(),
        ts.end(),
        append_iterator(rs),
        std::forward<F>(conv));
    return rs;
}

// TODO: requires guard
template <typename T>
std::optional<typename T::value_type> find(
    const T &ts,
    const auto &v) {
    auto it = std::find_if(
        ts.begin(), ts.end(), [&](const auto &u) { return u == v; });
    return it == ts.end() ? std::nullopt : std::make_optional(*it);
}

// TODO: requires guard
template <typename T, typename F>
std::optional<typename T::value_type> find(
    const T &ts,
    F &&f) {
    auto it = std::find_if(
        ts.begin(), ts.end(),
        [&](const auto &v) { return f(v); });
    return it == ts.end() ? std::nullopt : std::make_optional(*it);
}

template <typename T, typename F>
T &modify(T &ts, F &&f) {
    std::for_each(ts.begin(), ts.end(), std::forward<F>(f));
}

template <typename T, typename F>
T cmodify(const T &ts, F &&f) {
    T us = ts;
    std::for_each(us.begin(), us.end(), std::forward<F>(f));
    return us;
}

template <typename T, typename F>
T &transform(T &ts, F &&f) {
    std::for_each(ts.begin(), ts.end(), std::forward<F>(f));
}

template <template <typename...> class Container, typename T, typename F>
auto ctransform(const Container<T> &ts, F &&f) {
    using U = std::invoke_result_t<F, const T&>;
    Container<U> us;
    std::transform(
        ts.begin(),
        ts.end(),
        std::back_inserter(us),
        std::forward<F>(f));
    return us;
}

template <typename T>
    requires types::is_span<T>::value
std::span<const u8, std::dynamic_extent> bytes(const T &ts) {
    return std::span<const u8, std::dynamic_extent> {
        reinterpret_cast<const u8*>(&ts[0]),
        ts.size_bytes()
    };
}

template <typename T>
    requires (types::is_vector<T>::value
        || types::is_array<T>::value)
std::span<const u8, std::dynamic_extent> bytes(const T &ts) {
    return std::span<const u8, std::dynamic_extent> {
        reinterpret_cast<const u8*>(&ts[0]),
        sizeof(T::value_type) * ts.size()
    };
}
}

// hash for iterable
namespace std {
    template <typename It>
    struct hash<collections::iterable<It>> {
        size_t operator()(const collections::iterable<It> &it) const {
            u64 seed = 0;
            for (const auto &el : it) {
                u64 h = ::hash(el);
                h += 0x9E3779B97F4A7C15 + (seed << 6) + (seed >> 2);
                seed ^= h;
            }
            return seed;
        }
    };
}
