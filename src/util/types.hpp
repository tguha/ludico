#pragma once

#include "util/util.hpp"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef size_t usize;
typedef intmax_t isize;

typedef float f32;
typedef double f64;

#ifdef __SIZEOF_INT128__
#define HAS_INT128
#define HAS_INT128_BOOL true
typedef __int128_t i128;
typedef __uint128_t u128;
#elif
#define HAS_INT128_BOOL false
#endif

namespace types {
template<typename T, typename U>
constexpr std::ptrdiff_t offset_of(U T::*member) {
    return reinterpret_cast<std::ptrdiff_t>(
        &(reinterpret_cast<T const volatile*>(NULL)->*member));
}

// checks if T is instance of U with dynamic_cast
template <typename U, typename T>
inline bool instance_of(const T &t) {
    return dynamic_cast<const U*>(&t) != nullptr;
}

// checks if T is instance of U with dynamic_cast
template <typename U, typename T>
inline bool instance_of(const T *t) {
    return dynamic_cast<const U*>(t) != nullptr;
}

// check if two objects are the same type
template <typename T, typename U>
inline bool are_same(const T &t, const U &u) {
    return typeid(t) == typeid(u);
}

// used as dummy template parameter
struct Empty {};

// ::size is size of largest type
template <typename ...Ts>
struct SizeofLargest {
    static constexpr auto value = std::max({ sizeof(Ts)... });
};

template <typename T>
struct VoidToEmpty { using type = T; };

template <>
struct VoidToEmpty<void> { using Type = Empty; };

// stackoverflow.com/questions/28105077/how-can-i-get-the-class-of-a-member-function-pointer
template <typename> struct member_function_traits;

template <typename Return, typename Object, typename... Args>
struct member_function_traits<Return (Object::*)(Args...)> {
    typedef Return return_type;
    typedef Object instance_type;
    typedef Object & instance_reference;
    typedef std::tuple<Args...> arg_tuple;

   // Can mess with Args... if you need to, for example:
    static constexpr size_t argument_count = sizeof...(Args);
};

// If you intend to support const member functions you need another specialization.
template <typename Return, typename Object, typename... Args>
struct member_function_traits<Return (Object::*)(Args...) const> {
    typedef Return return_type;
    typedef Object instance_type;
    typedef Object const & instance_reference;
    typedef std::tuple<Args...> arg_tuple;

    // Can mess with Args... if you need to, for example:
    static constexpr size_t argument_count = sizeof...(Args);
};

template <typename> struct function_traits;

template <typename Return, typename ...Args>
struct function_traits<Return(Args...)> {
    typedef Return return_type;
    typedef std::tuple<Args...> arg_tuple;
    static constexpr size_t argument_count = sizeof...(Args);

    template <size_t i>
    struct arg {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

/* template <typename F> */
/* struct function_traits<std::function<F>> */
/*     : public function_traits<decltype(std::function<F>::operator())> { */
/* }; */

template<typename F>
auto arguments(const F &) -> typename function_traits<F>::arg_tuple;

template<typename F>
auto member_arguments(const F &) -> typename member_function_traits<F>::arg_tuple;

// stackoverflow.com/questions/16803814/how-do-i-return-the-largest-type-in-a-list-of-types
template <typename... Ts>
struct largest_type;

template <typename T>
struct largest_type<T>
{
    using type = T;
};

template <typename T, typename U, typename... Ts>
struct largest_type<T, U, Ts...>
{
    using type = typename largest_type<typename std::conditional<
            (sizeof(U) <= sizeof(T)), T, U
        >::type, Ts...
    >::type;
};

// template helpers
template <typename T>
concept is_record = std::is_class_v<T> || std::is_union_v<T>;

template <typename T>
struct is_unique_ptr : std::false_type {};

template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template<class T>
struct is_vector : std::false_type {};

template<class T>
struct is_vector<std::vector<T>> : std::true_type {};

template<class T>
struct is_set : std::false_type {};

template<class T>
struct is_set<std::set<T>> : std::true_type {};

template<class T>
struct is_unordered_set : std::false_type {};

template<class T>
struct is_unordered_set<std::unordered_set<T>> : std::true_type {};

template<class T>
struct is_deque : std::false_type {};

template<class T>
struct is_deque<std::deque<T>> : std::true_type {};

template<typename T>
struct is_span : std::false_type {};

template<typename T, size_t E>
struct is_span<std::span<T, E>> : std::true_type {};

template<typename T>
struct is_array : std::false_type {};

template<typename T, size_t E>
struct is_array<std::array<T, E>> : std::true_type {};

template<typename T>
struct is_initializer_list : std::false_type {};

template<typename T>
struct is_initializer_list<std::initializer_list<T>> : std::true_type {};

// from stackoverflow.com/questions/26351587/how-to-create-stdarray-with-initialization-list-without-providing-size-directl
template <typename... T>
constexpr auto make_array(T&&... values) ->
std::array<
    typename std::decay<
        typename std::common_type<T...>::type>::type,
    sizeof...(T)> {
    return std::array<
        typename std::decay<
            typename std::common_type<T...>::type>::type,
        sizeof...(T)>{std::forward<T>(values)...};
}

// FROM https://gist.github.com/Naios/86ccea2f577172c9f8b5

/// Identity class which is used to carry parameter packs.
template<typename... Args>
struct identity { };

namespace detail
{
    template<typename Function>
    struct unwrap_function_impl;

    template<typename _RTy, typename... _ATy>
    struct unwrap_function_impl<_RTy(_ATy...)>
    {
        /// The return type of the function.
        typedef _RTy return_type;

        /// The argument types of the function as pack in fu::identity.
        typedef identity<_ATy...> argument_type;

        /// The function provided as std::function
        typedef std::function<_RTy(_ATy...)> function_type;

        /// The function provided as function_ptr
        typedef _RTy(*function_ptr)(_ATy...);

        /// Argument type tuple
        typedef std::tuple<_ATy...> arg_tuple;

        template <size_t i>
        struct arg {
            typedef typename std::tuple_element<i, arg_tuple>::type type;
        };
    };

    /// STL: std::function
    template<typename _RTy, typename... _ATy>
    struct unwrap_function_impl<std::function<_RTy(_ATy...)>>
        : unwrap_function_impl<_RTy(_ATy...)> { };

    /// STL: std::tuple
    template<typename _RTy, typename... _ATy>
    struct unwrap_function_impl<std::tuple<_RTy, _ATy...>>
        : unwrap_function_impl<_RTy(_ATy...)> { };

    /// Function pointers
    template<typename _RTy, typename... _ATy>
    struct unwrap_function_impl<_RTy(*const)(_ATy...)>
        : unwrap_function_impl<_RTy(_ATy...)> { };

    /// Class Method pointers
    template<typename _CTy, typename _RTy, typename... _ATy>
    struct unwrap_function_impl<_RTy(_CTy::*)(_ATy...) const>
        : unwrap_function_impl<_RTy(_ATy...)> { };

    /// Pack in fu::identity
    template<typename _RTy, typename... _ATy>
    struct unwrap_function_impl<identity<_RTy, _ATy...>>
        : unwrap_function_impl<_RTy(_ATy...)> { };

    /// Unwrap through pointer of functor.
    template<typename Function>
    static auto select_best_unwrap_unary_arg(int)
        -> unwrap_function_impl<decltype(&Function::operator())>;

    /// Unwrap through plain type.
    template<typename Function>
    static auto select_best_unwrap_unary_arg(long)
        -> unwrap_function_impl<Function>;

    template<typename... _FTy>
    struct select_best_unwrap;

    /// Enable only if 1 template argument is given.
    template<typename _FTy>
    struct select_best_unwrap<_FTy>
    {
        typedef decltype(select_best_unwrap_unary_arg<_FTy>(0)) type;
    };

    // Enable if more then 1 template argument is given.
    // (Handles lazy typing)
    template<typename _RTy, typename... _ATy>
    struct select_best_unwrap<_RTy, _ATy...>
    {
        typedef unwrap_function_impl<_RTy(_ATy...)> type;
    };

    template <typename>
    struct to_true
        : std::true_type { };

    /// std::true_type if unwrappable
    template<typename... Function>
    static auto test_unwrappable(int)
        -> to_true<typename select_best_unwrap<Function...>::type::function_type>;

    /// std::false_type if not unwrappable
    template<typename... Function>
    static auto test_unwrappable(long)
        -> std::false_type;

 } // detail

/// Trait to unwrap function parameters of various types:
/// Function style definition: Result(Parameters...)
/// STL `std::function` : std::function<Result(Parameters...)>
/// STL `std::tuple` : std::tuple<Result, Parameters...>
/// C++ Function pointers: `Result(*)(Parameters...)`
/// C++ Class method pointers: `Result(Class::*)(Parameters...)`
/// Lazy typed signatures: `Result, Parameters...`
/// Also provides optimized unwrap of functors and functional objects.
template<typename... Function>
using unwrap_function =
    typename detail::select_best_unwrap<Function...>::type;

/// Trait which defines the return type of the function.
template<typename... Function>
using return_type_of_t =
    typename detail::select_best_unwrap<Function...>::type::return_type;

/// Trait which defines the argument types of the function packed in std::tuple.
template<typename... Function>
using argument_type_of_t =
    typename detail::select_best_unwrap<Function...>::type::argument_type;

/// Trait which defines the std::function type of the function.
template<typename... Function>
using function_type_of_t =
    typename detail::select_best_unwrap<Function...>::type::function_type;

/// Trait which defines the function pointer type of the function.
template<typename... Function>
using function_ptr_of_t =
    typename detail::select_best_unwrap<Function...>::type::function_ptr;

/// Trait which defines if the given function is unwrappable or not.
template<typename... Function>
struct is_unwrappable
    : decltype(detail::test_unwrappable<Function...>(0)) { };
}
