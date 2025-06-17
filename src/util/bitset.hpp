#pragma once

#include "util/types.hpp"
#include "util/util.hpp"

template <usize N>
struct Bitset {
    // num bytes
    static constexpr auto L = (N / 8) + ((N % 8) == 0 ? 0 : 1);

    // internal type
    using T =
        typename std::conditional<L == 1, u8,
            typename std::conditional<L == 2, u16,
                typename std::conditional<L <= 4, u32,
                    typename std::conditional<L <= 8, u64,
                        typename std::conditional<
                            HAS_INT128_BOOL,
                            u128, /* TODO: uint128 not faster? */
                            u64>::type>::type>::type>::type>::type;

    // number of bits per T
    static constexpr auto B = sizeof(T) * 8;

    // number of Ts in this bitset
    static constexpr auto K = (N / B) + ((N % B) == 0 ? 0 : 1);

    // numer of extra bits in last T
    static constexpr auto E = (B * K) - N;

    // mask for top T in array which zeroes its extra bits (if present)
    static constexpr T ZERO_EXTRA_MASK = (static_cast<T>(1) << (B - E)) - 1;

    // index iterator searching for bits == V
    template <bool V>
    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = u64;
        using difference_type = void;
        using pointer = void;
        using reference = void;

        explicit iterator(const Bitset<N> &bitset)
            : bitset(bitset),
              i(0) {
            if (this->bitset[this->i] != V) {
                this->seek();
            }
        }

        static inline iterator<V> end(const Bitset<N> &bitset) {
            return { bitset, N };
        }

        inline auto &operator++() {
            this->seek();
            return *this;
        }

        inline auto operator++(int) {
            auto t = *this;
            ++(*this);
            return t;
        }

        inline value_type operator*() const {
            return this->i;
        }

        inline bool operator==(const iterator<V> &rhs) const {
            return &this->bitset == &rhs.bitset && this->i == rhs.i;
        }

private:
        iterator(const Bitset<N> &bitset, usize i)
            : bitset(bitset),
              i(i) {}

        void seek() {
            // if word == skip_count, it can be skipped
            constexpr auto skip_count = V ? 0 : B;

            // j is current d[] index, k is last index where popcount was
            // checked (used to quickly skip bytes)
            usize j, k = std::numeric_limits<usize>::max();
            // v is current d[] value
            T v;
            do {
                this->i++;
loop:
                j = this->i / B;
                if (k != j) {
                    // update current word
                    k = j;
                    v = this->bitset.d[j];

                    // check if it can be skipped
                    if (std::popcount(v) == skip_count) {
                        if (j >= (K - 1)) {
                            this->i = N;
                            return;
                        }

                        // move to first bit of next word
                        this->i = (j + 1) * B;
                        goto loop;
                    }
                }
            } while ((!!(v & (T(1) << (this->i % B)))) != V && this->i < N);
        }

        const Bitset<N> &bitset;
        usize i;
    };

    inline auto begin_on() const { return iterator<true>(*this); }

    inline auto end_on() const { return iterator<true>::end(*this); }

    inline auto begin_off() const { return iterator<false>(*this); }

    inline auto end_off() const { return iterator<false>::end(*this); }

    template <typename I, typename V>
    inline void _and(I i, V v) {
        d[u64(i) / B] &= (T(v) << T(u64(i) % B));
    }

    template <typename I, typename V>
    inline void _or(I i, V v) {
        d[u64(i) / B] |= (T(v) << T(u64(i) % B));
    }

    template <typename I, typename V>
    inline void _xor(I i, V v) {
        d[u64(i) / B] |= (T(v) << T(u64(i) % B));
    }

    template <typename I>
    inline void set(I i) {
        d[u64(i) / B] |= (T(1) << T(u64(i) % B));
    }

    template <typename I>
    inline bool get(I i) const {
        return !!(d[u64(i) / B] & (T(1) << T(u64(i) % B)));
    }

    template <typename I>
    inline bool operator[](I i) const {
        return get(i);
    }

    template <typename I>
    inline void clear(I i) {
        d[u64(i) / B] &= ~((T(1) << T(u64(i) % B)));
    }

    // finds the n-th set bit in this bitmask
    // NOTE: 0-indexed!
    inline u64 nth_set(u64 n) const {
        // convert from 0-index to 1-index
        n++;

        u64 i = 0, r = 0, c;

        while (n > (c = std::popcount(this->d[i]))) {
            n -= c;
            i++;
            r += B;
        }

        // find the bit in this word
        // TODO: possibly faster method to do this
        u64 j = 0;
        const T v = this->d[i];
        while (n != 0) {
            if (v & (T(1) << j)) {
                n--;
            } else if (j == B - 1) {
                // failure
                return 0;
            }

            j++;
            r++;
        }

        // undo final increment for n = 0 loop, only return if not out of range
        const auto s = r - 1;
        return s >= N ? 0 : s;
    }

    inline void reset(bool value = false) {
        std::memset(&d, value ? 0xFF : 0, sizeof(T) * K);
    }

    inline usize popcnt(bool value = true) const {
        usize c = 0;
        for (usize i = 0; i < K; i++) {
            T mask = ~T(0);

            if constexpr (E != 0) {
                if (i == K - 1) {
                    mask = ZERO_EXTRA_MASK;
                }
            }

            c += std::popcount(this->d[i] & mask);
        }
        return value ? c : (N - value);
    }

    inline std::string to_string() const {
        std::string result;
        for (isize i = K - 1; i >= 0; i--) {
            result += std::bitset<B>(this->d[i]).to_string();
        }
        return result.substr(E, result.length() - E);
    }

    inline Bitset<N> operator~() const {
        Bitset<N> res = *this;
        #pragma omp simd
        {
            for (usize i = 0; i < K; i++) {
                res.d[i] = ~res.d[i];
            }
        }
        return res;
    }

    inline auto operator&=(const Bitset<N> &rhs) {
        #pragma omp simd
        {
            for (usize i = 0; i < K; i++) {
                this->d[i] &= rhs.d[i];
            }
        }
        return *this;
    }

    inline auto &operator|=(const Bitset<N> &rhs) {
        #pragma omp simd
        {
            for (usize i = 0; i < K; i++) {
                this->d[i] |= rhs.d[i];
            }
        }
        return *this;
    }

    inline auto operator^=(const Bitset<N> &rhs) {
        #pragma omp simd
        {
            for (usize i = 0; i < K; i++) {
                this->d[i] ^= rhs.d[i];
            }
        }
        return *this;
    }

    inline Bitset<N> operator&(const Bitset<N> &rhs) const {
        Bitset<N> res = *this;
        res &= rhs;
        return res;
    }

    inline Bitset<N> operator|(const Bitset<N> &rhs) const {
        Bitset<N> res = *this;
        res |= rhs;
        return res;
    }

    inline Bitset<N> operator^(const Bitset<N> &rhs) const {
        Bitset<N> res = *this;
        res ^= rhs;
        return res;
    }

    inline bool operator==(const Bitset<N> &rhs) const {
        // TODO: rewrite for vectorization?
        for (usize i = 0; i < K; i++) {
            if (this->d[i] != rhs.d[i]) {
                return false;
            }
        }
        return true;
    }

private:
    T d[K] = { 0 };
};
