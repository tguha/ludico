#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "util/log.hpp"

namespace ndarray {
// returns true if p is in bounds for ndarray of specified size
template <typename V>
inline bool in_bounds(const V &size, const V &p) {
    return math::all(math::lessThan(p, size))
        && math::all(math::greaterThanEqual(p, V(0)));
}

// n-dimensional vector for-loop
// traverses in order, so (fx.) an XYZ vector gets iterated as
// for (z in Z) for (y in Y) for (x in X) { ... }
template <typename V, typename F>
    requires (std::is_invocable_r_v<void, F, const V&>)
void each(const V &size, F &&f, const V &step = V(1), const V &init = V(0)) {
    ASSERT(math::all(math::notEqual(step, V(0))), "step must be nonzero");

    V p = init;

    while (true) {
        f(p);

        // step, starting with p[0]
        usize i = 0;
        while (true) {
            if (step[i] < 0) {
                if (p[i] >= 0) {
                    p[i] += step[i];
                    break;
                }

                p[i] = size[i] - 1;
            } else {
                p[i] += step[i];

                if (p[i] < size[i]) {
                    break;
                }

                p[i] = 0;
            }

            // finished when last dimension overflows
            if (i == math::vec_traits<V>::L - 1) {
                return;
            }

            i++;
        }
    }
}

// get element from an ndarray
template <typename V, usize L = math::vec_traits<V>::L>
inline auto &at(const V &size, auto *src, const V &idx) {
    if constexpr (L == 1) {
        return src[idx.x];
    } else if constexpr (L == 2) {
        return src[(idx.y * size.x) + idx.x];
    } else if constexpr (L == 3) {
        return src[(idx.z * size.y * size.x) + (idx.y * size.x) + idx.x];
    } else {
        constexpr auto N = math::vec_traits<V>::L;
        usize n = 0;

        for (usize i = 0; i < N; i++) {
            usize m = idx[i];
            for (usize j = i + 1; j < N; j++) {
                m *= size[j];
            }

            n += m;
        }

        return src[n];
    }
}

// convert an n-dimensional index into an ndarray into and n-dimensional
// position vector
template <typename V>
inline V unravel_index(const V &size, usize index) {
    V res;

    isize j = 0;
    for (isize i = math::vec_traits<V>::L - 1; i >= 0; i--) {
        res[j++] = index % size[i];
        index /= size[i];
    }

    return res;
}
}
