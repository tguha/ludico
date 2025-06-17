#include "util/util.hpp"

template class std::unordered_map<std::string, std::string>;
template class std::unordered_set<unsigned int>;
template class std::unordered_set<glm::vec4>;
template class std::vector<std::string>;

namespace fmt {
namespace detail {
template void vformat_to<char>(
    buffer<char>& buf, basic_string_view<char> fmt,
    basic_format_args<FMT_BUFFER_CONTEXT(type_identity_t<char>)> args,
    locale_ref loc);
}
}
