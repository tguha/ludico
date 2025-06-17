#pragma once

// stdlib headers
#include <iostream>
#include <cinttypes>
#include <cstdlib>
#include <limits>
#include <cstdint>
#include <cstddef>
#include <string>
#include <sstream>
#include <cstdbool>
#include <fstream>
#include <optional>
#include <vector>
#include <array>
#include <map>
#include <filesystem>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <deque>
#include <list>
#include <queue>
#include <stack>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <initializer_list>
#include <compare>
#include <type_traits>
#include <experimental/type_traits>
#include <variant>
#include <concepts>
#include <any>
#include <thread>
#include <mutex>
#include <span>
#include <random>
#include <typeindex>
#include <new>
#include <bit>

#define IN_UTIL_HPP
#include "util/glm.hpp"
#include "util/fmt.hpp"
#include "util/omp.hpp"
#undef IN_UTIL_HPP

// forward declarations of heavy templates
namespace fmt {
namespace detail {
extern template void vformat_to<char>(
    buffer<char>& buf, basic_string_view<char> fmt,
    basic_format_args<FMT_BUFFER_CONTEXT(type_identity_t<char>)> args,
    locale_ref loc);
}
}

extern template class std::unordered_map<std::string, std::string>;
extern template class std::unordered_set<unsigned int>;
extern template class std::unordered_set<glm::vec4>;
extern template class std::vector<std::string>;
