#include "util/stacktrace.hpp"

#include <backward.hpp>

std::string stacktrace::get() {
    using namespace backward;
    StackTrace st;
    st.load_here(32);

    std::stringstream ss;
    Printer p;
    p.print(st, ss);
    return ss.str();
}
