#pragma once

#include "gfx/util.hpp"
#include "util/collections.hpp"
#include "util/ndarray.hpp"
#include "util/rand.hpp"
#include "util/types.hpp"
#include "util/bitset.hpp"
#include "util/util.hpp"
#include "util/hash.hpp"
#include "util/assert.hpp"

namespace wfc {
// set to true to enable debug tracing
static constexpr auto DEBUG = false, VERBOSE = false;

template <usize N>
struct Neighbors {};

template <>
struct Neighbors<2> {
    static constexpr std::array<ivec2, 4> neighbors =
        {
            ivec2(-1, 0),
            ivec2(1, 0),
            ivec2(0, -1),
            ivec2(0, 1)
        };
};

template <>
struct Neighbors<3> {
    static constexpr std::array<ivec3, 6> neighbors =
        {
            ivec3(-1, 0, 0),
            ivec3(1, 0, 0),
            ivec3(0, -1, 0),
            ivec3(0, 1, 0),
            ivec3(0, 0, -1),
            ivec3(0, 0, 1)
        };
};

// permuate n-dimensional rotations of an array
template <usize N>
struct Rotator {};

template <>
struct Rotator<2> {
    using V = ivec2;

    // permute rotations, first index is ALWAYS src
    template <typename T, usize S>
    static inline std::array<std::array<T, S * S>, 4> permute(
        const std::array<T, S * S> &src) {
        std::array<std::array<T, S * S>, 4> dst;
        dst[0] = src;
        dst[1] = rotate_ccw<T, S>(dst[0]);
        dst[2] = rotate_ccw<T, S>(dst[1]);
        dst[3] = rotate_ccw<T, S>(dst[2]);
        return dst;
    }

private:
    template <typename T, usize S>
    static inline std::array<T, S * S> rotate_ccw(
        const std::array<T, S * S> &src) {
        std::array<T, S * S> dst;
        for (usize i = 0; i < S; i++) {
            for (usize j = 0; j < S; j++) {
                ndarray::at(V(S), &dst[0], { i, j })
                    = ndarray::at(V(S), &src[0], { S - j - 1, i });
            }
        }
        return dst;
    }
};

// permutes n-dimensional reflections of an array
template <usize N>
struct Reflector {
    using V = math::vec<N, int, math::defaultp>;

    // permute reflections, first index is ALWAYS src
    template <typename T, usize S, usize VOL = math::cexp::pow(S, N)>
    static inline auto permute(
        const std::array<T, VOL> &src) {
        std::array<std::array<T, VOL>, math::cexp::pow(2, N)> dst;

        // iterate over possible permutations of axes
        usize i = 0;
        ndarray::each(
            V(2),
            [&](const V &v) {
                const auto which = math::vec<N, bool, math::defaultp>(v);
                dst[i] = src;
                for (usize j = 0; j < N; j++) {
                    if (which[j]) {
                        dst[i] = reflect_axis<T, S>(dst[i], j);
                    }
                }
                i++;
            });

        return dst;
    }

private:
    template <typename T, usize S, usize VOL = math::cexp::pow(S, N)>
    static inline auto reflect_axis(
        const std::array<T, VOL> &src, usize axis) {
        std::array<T, VOL> dst;
        ndarray::each(
            V(S),
            [&](const V &v) {
                V u = v;
                u[axis] = S - u[axis] - 1;
                ndarray::at(V(S), &dst[0], v)
                    = ndarray::at(V(S), &src[0], u);
            });
        return dst;
    }
};

enum Flags {
    FLAG_NONE = 0,
    FLAG_ROTATE = 1 << 0,
    FLAG_REFLECT = 1 << 1
};

// which pattern function to use for WFC
enum class PatternFunction {
    RANDOM,
    WEIGHTED
};

// which next cell selection function to use for WFC
enum class NextCellFunction {
    MIN_ENTROPY
};

// behavior of borders for pattern creation:
// EXCLUDE: any pattern which would include a border is not used
// ZERO: borders have the value T(0)
// CLAMP: borders are clamped to their nearest defined value
// WRAP: borders are wrapped around the input data
enum class BorderBehavior {
    EXCLUDE,
    ZERO,
    CLAMP,
    WRAP
};

// implements n-dimensional wave function collapse using the "overlapping model"
// slot size is always 1 element, S is size of each overlapping pattern
// T: pattern type
// N: number of dimensions
// S: size of patterns (must be odd!)
template <
    typename T,
    usize N,
    usize S,
    usize D,
    typename V = math::vec<N, int, math::defaultp>,
    typename U = math::vec<N - 1,int, math::defaultp>>
    requires (S % 2 == 1)
struct WFC {
    struct Pattern;
    struct Element;
    struct Wave;

    // function which looks at current wave and returns the next cell to
    // collapse
    using NextCellFn = std::function<Element&(Wave&)>;

    // function which chooses the pattern when collapsing a cell
    using PatternFn = std::function<usize(Wave&, Element&)>;

    // function used various optional callbacks during collapse
    using CallbackFn = std::function<void(const Wave&)>;

    // S ** N (volume of one pattern)
    static constexpr auto VOL = math::cexp::pow(S, N);

    // pattern derived from input data
    struct Pattern {
        // unique pattern ID
        usize id;

        // frequency of occurrence for this pattern in input data
        f64 frequency;

        // value at the center of this pattern
        T value;

        // pattern data
        std::array<T, VOL> data;

        // valid neighbors on each side for this pattern
        std::array<Bitset<D>, 2 * N> valid;

        // hash of this pattern on its data
        inline u64 hash() const {
            if (this->_hash) {
                return this->_hash;
            }

            u64 v = 0x12345;
            for (const auto &i : this->data) {
                v ^= i + 0x9e3779b9 + (v << 6) + (v >> 2);
            }

            return this->_hash = v;
        }

        // spaceship comparison on pattern data
        auto operator<=>(const Pattern &other) const {
            return (*this) == other ?
                std::strong_ordering::equal
                : (this->hash() <=> other.hash());
        }

        // compare two patterns for equality on pattern data
        bool operator==(const Pattern &other) const {
            return this->hash() == other.hash() && this->data == other.data;
        }

private:
        // stored hash, calculated only once
        mutable u64 _hash = 0;
    };

    // output wave element
    struct Element {
        // position in output space
        V pos;

        // coefficient, marked bits are VALID pattern choices
        Bitset<D> c;

        // number of valid bits remaining
        usize popcnt;

        // memoized values
        // sum of weights of patterns in c
        f64 sum_weights = 0.0f;

        // sum of (weight * log(weight)) of patterns in c
        f64 sum_weight_log_weights = 0.0f;

        // calculated entropy of this pattern
        f64 entropy = 0.0f;

        // value post-collapse
        std::optional<T> value = std::nullopt;

        // initializes entropy values, mask
        void init(const Wave &w, const Bitset<D> &mask) {
            this->c = mask;
            this->popcnt = this->c.popcnt();
            for (
                auto it = this->c.begin_on();
                it != this->c.end_on();
                it++) {
                const auto weight = w.wfc.patterns[*it].frequency;
                this->sum_weights += weight;
                this->sum_weight_log_weights += weight * std::log(weight);
            }
        }

        // applies a mask to this element's coefficient, updating memoized
        // entropy values
        bool apply(const Wave &w, const Bitset<D> &mask) {
            // get bits which are on in the current c but off in the new mask
            // exit early if nothing changes
            const auto diff = this->c & (~mask);
            if (diff.popcnt() == 0) {
                return true;
            }

            this->c &= mask;

            for (
                auto it = diff.begin_on();
                it != diff.end_on();
                it++) {
                const auto weight = w.wfc.patterns[*it].frequency;
                this->sum_weights -= weight;
                this->sum_weight_log_weights -= weight * std::log(weight);
            }

            this->entropy =
                std::log(this->sum_weights)
                    - (this->sum_weight_log_weights / this->sum_weights);
            this->popcnt = this->c.popcnt();

            if (this->popcnt == 0) {
                return false;
            }

            return true;
        }

        // collapse this element to pattern n
        bool collapse(usize n, const T &value) {
            if (!this->c[n]) {
                return false;
            }

            this->value = value;
            this->c.reset();
            this->c.set(n);
            this->popcnt = 1;
            this->entropy = 0.0f;
            this->sum_weight_log_weights = 0.0f;
            this->sum_weights = 0.0f;

            return true;
        }

        // returns true if this element is collapsed
        bool collapsed() const {
            return static_cast<bool>(this->value);
        }
    };

    struct Wave {
        const WFC &wfc;

        // size of output wave
        V size_wave;

        // wave to collapse
        std::vector<Element> wave;

        // optional preset values
        const std::optional<T> *preset;

        // total number of collapsed elements
        usize num_collapsed = 0;

        Wave(
            const WFC &wfc,
            const V &size_wave,
            const std::optional<T> *preset = nullptr)
            : wfc(wfc),
              size_wave(size_wave),
              preset(preset) {
            if constexpr (DEBUG) {
                LOG("new wave of size {}", size_wave);
            }
        }

        // collapses specified wave element to only remaining possibility
        result::Result<void, V> collapse(
            Element &e,
            usize n = std::numeric_limits<usize>::max()) {
            if constexpr (DEBUG && VERBOSE) {
                LOG("collapsing {} ({})", e.pos, e.c);
            }

            n = (n == std::numeric_limits<usize>::max()) ? e.c.nth_set(0) : n;
            const auto &p = this->wfc.patterns[n];

            if (!e.collapse(n, p.value)) {
                return result::Err(e.pos);
            }

            if constexpr (DEBUG && VERBOSE) {
                LOG(
                    "  collapsed to {} (f={})",
                    *e.value,
                    p.value);
            }

            this->num_collapsed++;
            return result::Ok();
        }

        // observe specified wave element, collapsing it to one of its possible
        // values
        result::Result<void, V> observe(Element &e) {
            const auto n = this->wfc.pattern_fn(*this, e);
            if constexpr (DEBUG && VERBOSE) {
                LOG("observing {} to {} ({})", e.pos, n, e.c);
            }
            ASSERT(
                n < this->wfc.patterns.size()
                && this->wfc.mask_used[n]
                && e.c[n]);
            return this->collapse(e, n);
        }

        // propagate the current value of the specified wave element
        // returns Ok on success, erroneous position (with contradiction) on
        // failure
        result::Result<void, V> propagate(Element &to_propagate) {
            // DFS of elements to update
            std::stack<Element*> es;
            es.push(&to_propagate);

            // if not std::nullopt, there is an unsolveable contradiction in
            // the wave
            std::optional<V> contradiction = std::nullopt;

            while (!es.empty()) {
                auto &e = *es.top();
                es.pop();

                if constexpr (DEBUG && VERBOSE) {
                    LOG("propagating {} ({})", e.pos, e.c);
                }

                // get only valid/non-collapsed neighbors
                std::array<Element*, N * 2> neighbors;
                for (usize i = 0; i < N * 2; i++) {
                    neighbors[i] = nullptr;

                    const auto &n = Neighbors<N>::neighbors[i];
                    const auto pos_n = e.pos + n;

                    if (!ndarray::in_bounds(this->size_wave, pos_n)) {
                        continue;
                    }

                    auto &e_n =
                        ndarray::at(
                            this->size_wave,
                            &this->wave[0],
                            pos_n);

                    if (e_n.collapsed()) {
                        continue;
                    }

                    neighbors[i] = &e_n;
                }

                // compute "super"-patterns for necessary neighbors
                std::array<Bitset<D>, N * 2> neighbor_patterns;
                for (
                    auto it = e.c.begin_on();
                    it != e.c.end_on();
                    it++) {
                    const auto &p = this->wfc.patterns[*it];
                    for (usize i = 0; i < N * 2; i++) {
                        if (neighbors[i]) {
                            neighbor_patterns[i] |= p.valid[i];
                        }
                    }
                }

                for (usize i = 0; i < N * 2; i++) {
                    if (!neighbors[i]) {
                        continue;
                    }

                    auto &e_n = *neighbors[i];
                    const auto popcnt_old = e_n.popcnt;
                    e_n.apply(*this, neighbor_patterns[i]);

                    if (e_n.popcnt != popcnt_old) {
                        if (e_n.popcnt == 0) {
                            this->collapse(e_n, 0);
                            contradiction = e_n.pos;
                            break;
                        } else if (e_n.popcnt == 1) {
                            auto res = this->collapse(e_n);
                            if (res.isErr()) {
                                return res;
                            }
                        }

                        es.push(&e_n);
                    }
                }

                if (contradiction) {
                    if constexpr (DEBUG) {
                        LOG(
                            "contradiction at {}",
                            *contradiction);
                    }

                    return result::Err(*contradiction);
                }
            }

            if (this->wfc.on_propagate) {
                (*this->wfc.on_propagate)(*this);
            }

            return result::Ok();
        }

        // collapse the wave
        result::Result<void, V> collapse() {
            // initialize wave, all patterns are valid for every element
            this->wave = std::vector<Element>(math::prod(this->size_wave));

            // safe to parallelize because only one element is being modified at
            // a time
            #pragma omp parallel for
            for (usize i = 0; i < this->wave.size(); i++) {
                auto &e = this->wave[i];
                e.pos = ndarray::unravel_index(this->size_wave, i);
                e.init(*this, this->wfc.mask_used);
            }

            // load preset values if present by setting Element::value
            if (this->preset) {
                ndarray::each(
                    this->size_wave,
                    [&](const V &pos) {
                        const auto &p =
                            ndarray::at(this->size_wave, &this->preset[0], pos);
                        if (p) {
                            auto &e =
                                ndarray::at(
                                    this->size_wave,
                                    &this->wave[0],
                                    pos);
                            e.value = *p;
                        }
                    });
            }

            // collapse
            while (this->num_collapsed != this->wave.size()) {
                auto &e = this->wfc.next_cell_fn(*this);
                const auto res0 = this->observe(e);
                if (res0.isErr()) {
                    return res0;
                }

                const auto res1 = this->propagate(e);
                if (res1.isErr()) {
                    return res1;
                }
            }

            return result::Ok();
        }

        // dump current state wave to texture (tuple of [size, data])
        auto dump() const requires (N == 2) {
            const auto size_data = this->size_wave;
            std::vector<T> data(math::prod(size_data));

            for (isize y = 0; y < size_data.y; y++) {
                for (isize x = 0; x < size_data.x; x++) {
                    const auto pos = ivec2(x, y);
                    const auto &e =
                        ndarray::at(this->size_wave, &this->wave[0], pos);

                    usize count = 0;
                    u32 rgba[4] = { 0, 0, 0, 0 };
                    for (
                        auto it = e.c.begin_on();
                        it != e.c.end_on();
                        it++) {
                        const auto &p = this->wfc.patterns[*it];
                        count++;
                        for (usize i = 0; i < 4; i++) {
                            rgba[i] += (p.value >> (i * 8)) & 0xFF;
                        }
                    }

                    u32 value = 0;
                    for (usize i = 0; i < 4; i++) {
                        value |= ((rgba[i] / count) & 0xFF) << (i * 8);
                    }
                    data[(y * size_data.x) + x] = value;
                }
            }

            return std::make_tuple(size_data, data);
        }
    };

    // size on input pattern data
    V size_in;

    // input pattern data
    const T *in;

    // possible patterns
    std::vector<Pattern> patterns;

    // pattern function
    PatternFn pattern_fn;

    // next cell function
    NextCellFn next_cell_fn;

    // border behavior
    BorderBehavior border_behavior;

    // mask for used bits of coefficient bitsets
    // bits are also zeroed for disallowed patterns (patterns which have no
    // valid neighbors, etc.)
    Bitset<D> mask_used;

    // random generator
    Rand rand;

    // options
    usize flags;

    // optional calback to be called after propagation of each wave element
    std::optional<CallbackFn> on_propagate = std::nullopt;

    explicit WFC(
        const V &size_in,
        const T *in,
        PatternFunction pattern_function,
        NextCellFunction next_cell_function,
        BorderBehavior border_behavior,
        Rand &&rand,
        usize flags)
        : size_in(size_in),
          in(in),
          border_behavior(border_behavior),
          rand(std::move(rand)),
          flags(flags) {
        // removes equivalent patterns
        const auto deduplicate =
            [&]() {
                // deduplicate equivalent patterns
                const auto size_old = this->patterns.size();
                std::sort(this->patterns.begin(), this->patterns.end());
                this->patterns.erase(
                    std::unique(this->patterns.begin(), this->patterns.end()),
                    this->patterns.end());

                if constexpr (DEBUG) {
                    LOG(
                        "removed {} duplicate patterns ({} -> {})",
                        size_old - this->patterns.size(),
                        size_old,
                        this->patterns.size());
                }
            };

        // get pattern data from input at specified position
        // can return std::nullopt of location is made illegal by border
        // behavior
        const auto data_at =
            [&](const V &center) -> std::optional<std::array<T, VOL>> {
                const auto base = center - (V(S) / 2);

                if (border_behavior == BorderBehavior::EXCLUDE) {
                    if (!ndarray::in_bounds(size_in, base)
                            || !ndarray::in_bounds(size_in, base + V(S) - 1)) {
                        return std::nullopt;
                    }
                }

                std::array<T, VOL> dst;
                ndarray::each(
                    V(S),
                    [&](const V &offset) {
                        std::optional<T> override = std::nullopt;
                        auto pos = base + offset;

                        switch (border_behavior) {
                            case BorderBehavior::EXCLUDE:
                                break;
                            case BorderBehavior::ZERO:
                                if (!ndarray::in_bounds(size_in, pos)) {
                                    override = T(0);
                                }
                                break;
                            case BorderBehavior::CLAMP:
                                pos = math::clamp(pos, V(0), size_in - V(1));
                                break;
                            case BorderBehavior::WRAP:
                                pos = ((pos % size_in) + size_in) % size_in;
                                break;
                        };

                        ndarray::at(V(S), &dst[0], offset)
                            = override ?
                                *override
                                : ndarray::at(size_in, in, pos);
                    });
                return std::make_optional(dst);
            };

        // construct a pattern from the specified data
        const auto make_pattern =
            [&](const std::array<T, VOL> &data) -> Pattern {
                Pattern p;
                p.data = data;
                p.value = ndarray::at(V(S), &p.data[0], V(S) / 2);
                return p;
            };

        // slide along input data creating patterns
        ndarray::each(
            size_in,
            [&](const V &p) {
                if (const auto data = data_at(p)) {
                    this->patterns.emplace_back(make_pattern(*data));
                }
            });

        // calculate pattern frequencies (normalization occurs later)
        // NOTE: PER-HASH, not per-pattern, so this does not create issues when
        // deduplicating patterns
        std::unordered_map<u64, usize> pattern_hash_to_freq;
        for (const auto &p : this->patterns) {
            pattern_hash_to_freq[p.hash()]++;
        }

        for (auto &p : this->patterns) {
            p.frequency = static_cast<f64>(pattern_hash_to_freq[p.hash()]);
        }

        // 1st deduplication before permutations
        deduplicate();

        // keep a copy of base patterns for rotations and reflections
        const auto base_patterns = this->patterns;

        // add rotations and reflections, recalculate frequencies
        this->patterns.clear();
        for (auto &p : base_patterns) {
            this->patterns.push_back(p);

            // add rotations
            if (flags & FLAG_ROTATE) {
                ASSERT(N == 2, "can only rotate patterns in 2 dimensions");

                if constexpr (N == 2) {
                    const auto permutations =
                        Rotator<N>::template permute<T, S>(p.data);
                    for (usize i = 1; i < permutations.size(); i++) {
                        auto &q =
                            this->patterns.emplace_back(
                                make_pattern(permutations[i]));
                        q.frequency = p.frequency;
                    }
                }
            }

            // add reflections
            if (flags & FLAG_REFLECT) {
                const auto permutations =
                    Reflector<N>::template permute<T, S>(p.data);
                for (usize i = 1; i < permutations.size(); i++) {
                    auto &q =
                        this->patterns.emplace_back(
                            make_pattern(permutations[i]));
                    q.frequency = p.frequency;
                }
            }
        }

        // remove equivalent permutations
        deduplicate();

        // normalize pattern frequencies
        f32 frequency_total = 0.0f;
        for (const auto &p : this->patterns) {
            frequency_total += p.frequency;
        }

        for (auto &p : this->patterns) {
            p.frequency /= frequency_total;
        }

        ASSERT(
            this->patterns.size() <= D,
            "D ({}) is not large enough (must be at least {} bits)",
            D,
            this->patterns.size());

        if (this->patterns.size() < (D / 2)) {
            WARN(
                "consider shrinking D ({}), only {} bits are needed",
                D,
                this->patterns.size());
        }

        // assign IDs, calculate used mask
        this->mask_used.reset();
        for (usize i = 0; i < this->patterns.size(); i++) {
            this->patterns[i].id = i;
            this->mask_used.set(i);
        }

        // compute valid patterns around each pattern (adjacency)
        // check against all overlapping slots for each pattern pair (p, q) for
        // every side
        //
        // safe to parallelize because only one pattern is modified by each
        // thread
        #pragma omp parallel for
        for (auto &p : this->patterns) {
            for (const auto &q : patterns) {
                for (usize i = 0; i < 2 * N; i++) {
                    const auto &n = Neighbors<N>::neighbors[i];
                    bool valid = true;
                    ndarray::each(
                        V(S),
                        [&](const auto &offset_q) {
                            if (!valid) {
                                return;
                            }

                            // compute offset into p's data
                            const auto offset_p = n + offset_q;
                            if (!ndarray::in_bounds(V(S), offset_p)) {
                                return;
                            }

                            const auto
                                v_p =
                                    ndarray::at(V(S), &p.data[0], offset_p),
                                v_q =
                                    ndarray::at(V(S), &q.data[0], offset_q);

                            if (v_p != v_q) {
                                valid = false;
                            }
                        });

                    if (valid) {
                        p.valid[i].set(q.id);
                    }
                }
            }
        }

        if constexpr (DEBUG) {
            LOG("patterns (size {}):", this->patterns.size());

            if constexpr (VERBOSE) {
                for (auto &p : this->patterns) {
                    LOG("  {}:", p.id);
                    LOG("    {}", p.frequency);
                    for (usize i = 0; i < p.valid.size(); i++) {
                        LOG("    {}", p.valid[i]);
                    }
                }
            }
        }

        // pick pattern function
        switch(pattern_function) {
            case PatternFunction::RANDOM:
                this->pattern_fn = this->pattern_random(); break;
            case PatternFunction::WEIGHTED:
                this->pattern_fn = this->pattern_weighted(); break;
        }

        // pick next cell function
        switch(next_cell_function) {
            case NextCellFunction::MIN_ENTROPY:
                this->next_cell_fn = this->next_cell_min_entropy(); break;
        }

        if constexpr (DEBUG) {
            // emit info about frequency of each T in source data
            usize count_patterns = 0, count_in = 0;
            f64 total_freq = 0.0f;
            std::unordered_map<T, usize> t_to_count_patterns;
            std::unordered_map<T, f64> t_to_freq_patterns;
            std::unordered_map<T, usize> t_to_count_in;

            for (const auto &p : this->patterns) {
                t_to_count_patterns[p.value]++;
                t_to_freq_patterns[p.value] += p.frequency;
                total_freq += p.frequency;
                count_patterns++;
            }

            for (isize i = 0; i < math::prod(size_in); i++) {
                t_to_count_in[in[i]]++;
                count_in++;
            }

            ASSERT(t_to_count_patterns.size() == t_to_count_in.size());

            for (const auto &[t, c] : t_to_count_patterns) {
                LOG(
                    "T: {}, {:.2f} / {:.2f} / {:.2f}",
                    t,
                    t_to_count_in[t] / static_cast<f64>(count_in),
                    c / static_cast<f64>(count_patterns),
                    t_to_freq_patterns[t] / total_freq);
            }
        }
    }

    // returns [size, data] of valid patterns in this WFC instance
    // can be used to output an image if data is RGBA, etc.
    auto dump_patterns() const requires (N == 2) {
        const auto size = uvec2(this->patterns.size() * S, S);
        std::vector<T> data(math::prod(size));

        for (usize i = 0; i < this->patterns.size(); i++) {
            const auto &p = this->patterns[i];
            ndarray::each(
                V(S),
                [&](const V &offset) {
                    ndarray::at(
                        size,
                        &data[0],
                        { (i * S) + offset.x, offset.y })
                        = ndarray::at(
                            V(S),
                            &p.data[0],
                            offset);
                });
        }

        return std::make_tuple(size, data);
    }

    // collapses a wave of size "size_out" to "out"
    // returns true on success
    bool collapse(
        const V &size_out,
        T *out,
        const std::optional<T> *preset = nullptr) const {
        auto w = Wave(*this, size_out, preset);

        const auto res = w.collapse();

        if (res.isErr()) {
            LOG("contradiction at {}", res.unwrapErr());
            return false;
        }

        // map elements to output
        ndarray::each(
            w.size_wave,
            [&](const V &pos) {
                ndarray::at(size_out, out, pos)
                    = *ndarray::at(size_out, &w.wave[0], pos).value;
            });

        if constexpr (DEBUG) {
            // emit info about frequency of each T in output
            usize count = 0;
            std::unordered_map<T, usize> t_to_count;
            for (isize i = 0; i < math::prod(size_out); i++) {
                t_to_count[out[i]]++;
                count++;
            }

            for (const auto &[t, c] : t_to_count) {
                LOG("OUTPUT:");
                LOG("  T: {}, {:.2f}", t, c / static_cast<f64>(count));
            }
        }

        return true;
    }

    // returns a function which selects a random pattern
    PatternFn pattern_random() {
        return [&](Wave &w, Element &e) -> usize {
            ASSERT(e.popcnt != 0 && e.c.popcnt() != 0);
            const auto i = this->rand.template next<usize>(0, e.c.popcnt() - 1);
            const auto n = e.c.nth_set(i);
            ASSERT(n < this->patterns.size());
            return n;
        };
    }

    // returns a function which selects a pattern based on the distribution of
    // patterns in the input data
    PatternFn pattern_weighted() {
        // pick a random index of the remaining coefficients
        // weighted by the frequencies in the sample data
        return [&](Wave &w, Element &e) -> usize {
            f64 sum_cs = 0.0f;
            std::vector<f64> cs(this->patterns.size());
            for (
                auto it = e.c.begin_on();
                it != e.c.end_on();
                it++) {
                cs[*it] = this->patterns[*it].frequency;
                sum_cs += cs[*it];
            }

            const auto r = this->rand.template next<f64>(0.0f, sum_cs);
            f64 acc = 0.0f;
            for (usize i = 0; i < cs.size(); i++) {
                acc += cs[i];

                if (acc >= r) {
                    return i;
                }
            }

            if constexpr (DEBUG) {
                LOG("failed weighted collapse for {} {}", e.pos, e.c);
            }

            ASSERT(false, "failed to select pattern for {}", e.pos);
            return e.c.nth_set(0);
        };
    }

    // returns a function which selects the next cell based on finding the cell
    // with minimum entropy in those remaining
    NextCellFn next_cell_min_entropy() {
        return [&](Wave &w) -> Element& {
            f64 min = 1e4;
            Element *argmin = nullptr;

            for (auto &e : w.wave) {
                if (!e.collapsed()
                        && e.entropy < min
                        && e.entropy + rand.next<f64>(0.0f, 1e-6) < min) {
                    argmin = &e;
                }
            }

            return *argmin;
        };
    }
};
}
