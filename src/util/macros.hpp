#pragma once

#define GET_MACRO(_0,_1,_2,NAME,...) NAME

// potentially compiler-specific macros
#define UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))
#define ALIGNED(_n) __attribute__((aligned(_n)))

#define __STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) __STRINGIFY_IMPL(x)
#define PRAGMA(x) _Pragma(#x)

#if defined(__clang__)
#define UNROLL(n) PRAGMA(clang loop unroll_count(n))
#elif defined(__GNU__)
#define UNROLL(n) PRAGMA(GCC unroll n)
#else
#define UNROLL(n) PRAGMA(unroll)
#endif

// move elsewhere
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

// see:
// stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
// get number of arguments with __NARG__
#define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define __RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0

#define NARG __NARG__

// general definition for any function name
#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n) _VFUNC_(name, n)
#define VFUNC(func, ...) _VFUNC(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__)

// FUN_EACH macro
// apply a function to up to 8 arguments
// usage:
// FUN_EACH(MACRO, first arg/i, second arg/j, things to apply macro to...)
// (adapted from https://indiegamedev.net/2022/03/28/automatic-serialization-in-cpp-for-game-engines/)
#define _FUN_EACH_1(_fn, _i, _j, _x)                                           \
    _fn(_i, _j, _x)

#define _FUN_EACH_2(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_1(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_3(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_2(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_4(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_3(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_5(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_4(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_6(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_5(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_7(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_6(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_8(_fn, _i, _j, _x, ...)                                      \
    _fn(_i, _j, _x);                                                           \
    _FUN_EACH_7(_fn, _i, _j, __VA_ARGS__)

#define _FUN_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define _FUN_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__)
#define _FUN_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define _FUN_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0
#define _FUN_EACH_IMPL(N, _fn, ...) CONCAT(FOR_EACH_, N)(_fn, __VA_ARGS__)

#define FUN_EACH(_fn, _i, _j, ...) \
    _FUN_EACH_IMPL(_FOR_EACH_NARG(__VA_ARGS__), _fn, _i, _j, __VA_ARGS__)

#define CONCAT_ARGS_1(x) #x
#define CONCAT_ARGS_2(x, ...) #x CONCAT_ARGS_1(__VA_ARGS__)
#define CONCAT_ARGS_3(x, ...) #x CONCAT_ARGS_2(__VA_ARGS__)
#define CONCAT_ARGS_4(x, ...) #x CONCAT_ARGS_3(__VA_ARGS__)
#define CONCAT_ARGS_5(x, ...) #x CONCAT_ARGS_4(__VA_ARGS__)
#define CONCAT_ARGS_6(x, ...) #x CONCAT_ARGS_5(__VA_ARGS__)
#define CONCAT_ARGS_7(x, ...) #x CONCAT_ARGS_6(__VA_ARGS__)
#define CONCAT_ARGS_8(x, ...) #x CONCAT_ARGS_7(__VA_ARGS__)

#define _NUM_ARGS2(X,X8,X7,X6,X5,X4,X3,X2,X1,N,...) N
#define NUM_ARGS(...) _NUM_ARGS2(0, __VA_ARGS__ ,8,7,6,5,4,3,2,1,0)

#define CONCAT_ARGS_N3(N, ...) CONCAT_ARGS_ ## N(__VA_ARGS__)
#define CONCAT_ARGS_N2(N, ...) CONCAT_ARGS_N3(N, __VA_ARGS__)
#define CONCAT_ARGS_N(...)     CONCAT_ARGS_N2(NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
