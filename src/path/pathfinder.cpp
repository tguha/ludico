#include "path/pathfinder.hpp"
#include "util/direction.hpp"

struct Node {
    ivec3 pos;
    f32 cost;

    auto operator<=>(const Node &other) const {
        return this->cost <=> other.cost;
    }
};

// cost to get to b from a
f32 Pathfinder::distance_cost(const ivec3 &a, const ivec3 &b) {
    return math::length(vec3(a) - vec3(b));
}

std::optional<Path> Pathfinder::find(
    const ivec3 &start,
    const ivec3 &end,
    usize limit,
    CanPathFn can_path_fn,
    CostFn cost_fn) {

    std::unordered_map<ivec3, ivec3> from;
    std::unordered_map<ivec3, f32> cost;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> frontier;

    frontier.push(Node { start, 0.0f });
    from[start] = start;
    cost[start] = 0.0f;

    // number of processed nodes
    usize iterations = 0;

    while (!frontier.empty()) {
        const auto n = frontier.top();
        iterations++;

        frontier.pop();

        if (n.pos == end || iterations >= limit) {
            // rebuild path
            Path path;
            ivec3 p;

            path.add(n.pos);
            while ((p = path[path.size() - 1]) != start) {
                path.add(from[p]);
            }
            path.reverse();
            return path;
        }

        for (const auto &d : Direction::ALL) {
            const auto next = n.pos + Direction::to_ivec3(d);

            if (!can_path_fn(n.pos, next)) {
                continue;
            }

            const auto cost_next = cost[n.pos] + cost_fn(n.pos, next);

            auto it = cost.find(next);
            if (it == cost.end() || cost_next < (*it).second) {
                cost.insert(it, std::make_pair(next, cost_next));
                frontier.push(Node { next, cost_fn(next, end) });
                from[next] = n.pos;
            }
        }
    }

    return std::nullopt;
}
