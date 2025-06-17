#include "test.hpp"
#include "util/allocator.hpp"
#include "util/basic_allocator.hpp"
#include "util/bump_allocator.hpp"
#include "util/stack_allocator.hpp"
#include "util/list.hpp"

template <typename F, typename L = std::invoke_result_t<F>>
    requires std::is_same_v<typename L::value_type, int>
static void test_int(F &&factory) {
    L l = factory();
    ASSERT(l.size() == 0);

    l.push(1);
    l.push(2);
    l.push(3);
    ASSERT(l.size() == 3);

    l.push(l);
    ASSERT(l.size() == 6);

    ASSERT(l.pop() == 3);
    ASSERT(l.pop() == 2);
    ASSERT(l.size() == 4);

    l.clear();
    ASSERT(l.size() == 0);

    for (usize i = 0; i < 256; i++) {
        l.push(i);
    }

    for (usize i = 50; i < 100; i++) {
        l.remove(50);
    }

    for (usize i = 0; i < l.size(); i++) {
        ASSERT(
            l[i] <= 49
            || l[i] >= 100);
    }

    usize i = 0;
    for (const auto &k : l) {
        ASSERT(k == l[i++]);
        ASSERT(
            k <= 49
            || k >= 100);
    }

    l.insert(50, 789);
    ASSERT(l[49] == 49);
    ASSERT(l[50] == 789);
    ASSERT(l[51] == 100);

    for (int i = 255; i >= (255 - 60); i--) {
        ASSERT(l.pop() == i);
    }
}

struct MyListConfig {
    static constexpr auto default_size = 1, multiple = 1;
};

void test_allocator(Allocator *alloc, bool use_lots_of_memory = true) {
    test_int([&]() { return List<int>(alloc); });
    test_int([&]() { return FixedList<int, 1024>(); });
    test_int([&]() { return BlockList<int, 2>(alloc); });
    test_int([&]() { return BlockList<int, 4>(alloc); });
    test_int([&]() { return BlockList<int, 8>(alloc); });
    test_int([&]() { return BlockList<int, 256>(alloc); });
    test_int([&]() { return BlockList<int, 1>(alloc); });
    test_int([&]() { return SmallList<int, 1>(alloc); });
    test_int([&]() { return SmallList<int, 3>(alloc); });
    test_int([&]() { return SmallList<int, 8>(alloc); });

    if (use_lots_of_memory) {
        test_int([&]() { return SmallList<int, 16>(alloc); });
        test_int([&]() { return SmallList<int, 32>(alloc); });
        test_int([&]() { return SmallList<int, 64>(alloc); });
        test_int([&]() { return SmallList<int, 75>(alloc); });
        test_int([&]() { return List<int, MyListConfig>(alloc); });
    }
}

static int global_dtor_count = 0;

struct MoveMe {
    int x = 0;

    MoveMe(int x = 0) : x(x) {}
    MoveMe(const MoveMe&) = delete;
    MoveMe &operator=(const MoveMe&) = delete;

    MoveMe(MoveMe &&other) { *this = std::move(other); }

    MoveMe &operator=(MoveMe &&other) {
        this->x = other.x == 0 ? 0 : (other.x + 1);
        other.x = 0;
        return *this;
    }

    ~MoveMe() {
        if (x != 0) { global_dtor_count++; }
    }
};

template <typename L>
    requires std::is_same_v<typename L::value_type, MoveMe>
void test_moves_basic(L &&l0) {
    global_dtor_count = 0;
    {
        auto l = std::move(l0);
        ASSERT(l.push(MoveMe(1)).x == 2);

        for (usize i = 0; i < 64; i++) {
            ASSERT(l.push(MoveMe(1)).x == 2);
        }

        ASSERT(l.pop().x == 4);
        ASSERT(l.size() == 64);
    }
    ASSERT(global_dtor_count == 65);
}

template <typename L>
    requires std::is_same_v<typename L::value_type, MoveMe>
void test_moves(L &&l0) {
    auto l = std::move(l0);
    ASSERT(l.push(MoveMe(1)).x == 2);

    l.push(MoveMe(1));
    l[1].x = 12;
    ASSERT(l.pop().x > 13);

    auto g = std::move(l);
    ASSERT(g[0].x == 3);
    g[0].x = 4;

    // force reallocations
    for (usize i = 0; i < 100; i++) {
        g.push(MoveMe(4));
    }
    ASSERT(g[0].x > g[16].x);
    ASSERT(g[16].x > g[100].x);
}

int main(int argc, char *argv[]) {
    {
        BasicAllocator alloc_base;
        BumpAllocator alloc_bump(&alloc_base, 16384);
        test_allocator(&alloc_base);
        test_allocator(&alloc_bump);
    }

    {
        StackAllocator<65536> alloc_stack;
        test_allocator(&alloc_stack, false);
    }

    BasicAllocator alloc_basic;
    test_moves(List<MoveMe>(&alloc_basic));
    test_moves_basic(List<MoveMe>(&alloc_basic));
    test_moves_basic(BlockList<MoveMe, 1>(&alloc_basic));
    test_moves_basic(BlockList<MoveMe, 2>(&alloc_basic));
    test_moves_basic(BlockList<MoveMe, 4>(&alloc_basic));
    test_moves_basic(SmallList<MoveMe, 4>(&alloc_basic));
    test_moves_basic(FixedList<MoveMe, 128>());

    return 0;
}
