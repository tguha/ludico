#define ASSERT(...)

#include "test.hpp"
#include "util/map.hpp"
#include "util/basic_allocator.hpp"
#include "util/rand.hpp"

template <typename>
struct is_map : std::false_type {};

template <typename K, typename V, typename R, typename H>
struct is_map<Map<K, V, R, H>> : std::true_type {};

static BasicAllocator allocator;

// current time nanos
u64 now() {
    return
        std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
}

template <typename M, usize REPEAT>
std::tuple<u64, u64> bench(usize count, Rand &rand) {
    constexpr auto IS_MY_MAP = is_map<M>::value;

    std::array<std::tuple<u64, u64>, REPEAT> rs;

    for (usize i = 0; i < REPEAT; i++) {
        u64 time_insert, time_lookup;

        M m;

        if constexpr (IS_MY_MAP) {
            m = M(&allocator);
        }

        u64 t0 = now();

        for (usize k = 0; k < count; ++k) {
            const auto n = rand.next<usize>();
            if constexpr (IS_MY_MAP) {
                m.insert(n, n - 1);
            } else {
                m.emplace(n, n - 1);
            }
        }

        time_insert = now() - t0;
        t0 = now();

        usize sum = 0;
        for (usize k = 0; k < count; ++k) {
            const auto n = rand.next<usize>();
            auto it = m.find(n);

            if constexpr (IS_MY_MAP) {
                if (it != m.end()) { sum += it->value; }
            } else {
                if (it != m.end()) { sum += it->second; }
            }
        }

        time_lookup = now() - t0;
        rs[i] = std::make_tuple(time_insert, time_lookup);
    }


    u64 t0 = 0, t1 = 0;
    for (usize i = 0; i < rs.size(); i++) {
        t0 += std::get<0>(rs[i]);
        t1 += std::get<1>(rs[i]);
    }
    return std::make_tuple(t0 / REPEAT, t1 / REPEAT);
}

int main(int argc, char *argv[]) {
    constexpr auto REPEAT = 5;

    constexpr auto WIDTH = 15;
    const std::string header =
        "|     size      |   operation   | unordered_map |      map      |      Map      |";

    const auto leftpad = [](const auto &x) {
        const auto s = fmt::format("{}", x);

        if (s.length() >= WIDTH) {
            return s;
        }

        return std::string(WIDTH - s.length(), ' ') + s;
    };

    const auto format_nanos = [](u64 ns) {
        return fmt::format("{:.3f} ms", static_cast<f64>(ns) / 1000000.0);
    };

    std::vector<usize> is;
    std::vector<std::tuple<u64, u64, u64>> times_insert, times_lookup;

    for (usize i = 1; i < 1000000; i <<= 1) {
        LOG("i = {}", i);
        is.push_back(i);

        std::array<std::tuple<u64, u64>, 3> times;
        Rand r(i);
        times[0] = bench<std::unordered_map<usize, usize>, REPEAT>(i, r);
        times[1] = bench<std::map<usize, usize>, REPEAT>(i, r);
        times[2] = bench<Map<usize, usize>, REPEAT>(i, r);

        times_insert.emplace_back(
            std::get<0>(times[0]),
            std::get<0>(times[1]),
            std::get<0>(times[2]));

        times_lookup.emplace_back(
            std::get<1>(times[0]),
            std::get<1>(times[1]),
            std::get<1>(times[2]));

    }


    LOG("{}", header);
    LOG("{}", std::string(header.length(), '-'));

    const auto print_times =
        [&](const std::string &name, const auto &v) {
            const auto greenify =
                [](const std::string &s) {
                    return fmt::format("\033[32;1m{}\033[0m", s);
                };

            const auto identity =
                [](const std::string &s) { return s; };

            usize i = 0;
            for (const auto &[t0, t1, t2] : v) {
                const auto least =
                    t0 < t1 ? (t0 < t2 ? 0 : 2) : (t1 < t2 ? 1 : 2);
                LOG(
                    "|{}|{}|{}|{}|{}|",
                    leftpad(is[i++]),
                    leftpad(name),
                    (least == 0 ? greenify : identity)(leftpad(format_nanos(t0))),
                    (least == 1 ? greenify : identity)(leftpad(format_nanos(t1))),
                    (least == 2 ? greenify : identity)(leftpad(format_nanos(t2))));
            }
        };

    print_times("insert", times_insert);
    print_times("lookup", times_lookup);

    return 0;
}
