#include "test.hpp"

#include "util/map.hpp"
#include "util/basic_allocator.hpp"

int main(int argc, char *argv[]) {
    BasicAllocator allocator;
    Map<int, int> a(&allocator);
    ASSERT(a.size() == 0);
    a.insert(2, 3);
    ASSERT(a.size() == 1);
    ASSERT(a[2] == 3);
    a.insert(2, 4);
    ASSERT(a.size() == 1);
    ASSERT(a[2] == 4);

    for (usize i = 0; i < 128; i++) {
        a.insert(i, i + 1);
    }

    ASSERT(a.remove(3)->value == 4);
    ASSERT(a.size() == 127);
    ASSERT(a.find(3) == a.end());

    for (usize i = 0; i < 100; i++) {
        if (i % 2) {
            a.remove(i);
        }
    }

    for (const auto &[k, v] : a) {
        ASSERT(k < 100 ? !(k % 2) : true);
        ASSERT(v == k + 1);
    }

    ASSERT((Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }})[3] == 4);
    ASSERT((Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }})[5] == 6);
    ASSERT(!(Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}).contains(77));

    auto b = (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }});

    ASSERT(a == a);
    ASSERT(b == b);
    ASSERT(a != b);
    ASSERT(a != (Map<int, int> { &allocator }));
    ASSERT(b != (Map<int, int> { &allocator }));
    ASSERT(a != (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}));
    ASSERT(
        (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}) ==
            (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}));

    ASSERT(a != (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}));
    ASSERT(b == (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}));
    ASSERT(
        (Map<int, int> { &allocator, { { 3, 4 }, { 5, 6 } }}) !=
            (Map<int, int> { &allocator, { { 3, 4 }, { 5, 20 } }}));

    b.merge(b);
    ASSERT(b == b);
    ASSERT(b.size() == 2);
    ASSERT(b[3] == 4);
    ASSERT(b[5] == 6);
    b[5] += 10;
    ASSERT(b[5] == 16);

    for (usize i = 0; i < 256; i++) {
        if (i != 5) {
            b.insert(i, i * i);
        }
    }

    ASSERT(b[5] == 16);

    Map<std::string, std::string> ss(&allocator);
    ss.insert("Bob", "+1 (456) 789-1110");
    ss.insert("John", "+1 (243) 586-4355");
    ss.insert("Jane", "+1 (693) 999-9999");
    ASSERT(ss.size() == 3);
    Map<std::string, std::string> ts = ss;
    Map<std::string, std::string> vs = std::move(ss);
    ASSERT(ss.size() == 0);
    ASSERT(ts.size() == 3);
    ASSERT(vs.size() == 3);
    ASSERT(ts == vs);

    Map<int, std::unique_ptr<std::string>> ps(&allocator);
    ps.insert(0, std::make_unique<std::string>("hello"));
    ps.insert(1, std::make_unique<std::string>("this"));
    ps.insert(2, std::make_unique<std::string>("is"));
    ps.insert(3, std::make_unique<std::string>("my"));
    ps.insert(5885, std::make_unique<std::string>("map"));
    ASSERT(*ps[0] == "hello");
    ASSERT(*ps[5885] == "map");
    return 0;
}
