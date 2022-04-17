#ifndef SIMPLE_MACRO_H
#define SIMPLE_MACRO_H

#define PP_THIRD_ARG(a, b, c, ...) c
#define VA_OPT_SUPPORTED_I(...) PP_THIRD_ARG(__VA_OPT__(, ), 1, 0, )
#define VA_OPT_SUPPORTED VA_OPT_SUPPORTED_I(?)

// after platform vs2019 version 16.6 Preview 2 complier with /Zc
// https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/
// Traditional MSVC requires a special EXPAND phase
#if (defined(_MSC_VER) && !defined(_MSVC_TRADITIONAL)) || \
    (defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL)

#define GET_ARG_COUNT(...) \
    INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(x) x
#define INTERNAL_EXPAND_ARGS_PRIVATE(...)                                 \
    INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(                       \
        __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, \
        87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72,   \
        71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56,   \
        55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40,   \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24,   \
        23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7,  \
        6, 5, 4, 3, 2, 1, 0))

#else // Other compilers

#if VA_OPT_SUPPORTED // Standardized in C++20
#define GET_ARG_COUNT(...)                                              \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                     \
        unused __VA_OPT__(, ) __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, \
        93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, \
        77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, \
        61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, \
        45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, \
        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#elif defined(__GNUC__) // Extension in GCC/Clang
#define GET_ARG_COUNT(...)                                              \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                     \
        unused, ##__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, \
        90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, \
        74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, \
        58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, \
        42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, \
        26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
        10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else // GET_ARG_COUNT() may return 1 here
#define GET_ARG_COUNT(...)                                                 \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                        \
        unused, __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90,  \
        89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74,    \
        73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58,    \
        57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42,    \
        41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,    \
        25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, \
        8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#endif

#define INTERNAL_GET_ARG_COUNT_PRIVATE(                                   \
    e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15, \
    e16, e17, e18, e19, e20, e21, e22, e23, e24, e25, e26, e27, e28, e29, \
    e30, e31, e32, e33, e34, e35, e36, e37, e38, e39, e40, e41, e42, e43, \
    e44, e45, e46, e47, e48, e49, e50, e51, e52, e53, e54, e55, e56, e57, \
    e58, e59, e60, e61, e62, e63, e64, e65, e66, e67, e68, e69, e70, e71, \
    e72, e73, e74, e75, e76, e77, e78, e79, e80, e81, e82, e83, e84, e85, \
    e86, e87, e88, e89, e90, e91, e92, e93, e94, e95, e96, e97, e98, e99, \
    e100, count, ...)                                                     \
    count

#define _RFEL_REPEAT_0(func, i, arg)
#define _RFEL_REPEAT_1(func, i, arg) func(i, arg)
#define _RFEL_REPEAT_2(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_1(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_3(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_2(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_4(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_3(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_5(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_4(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_6(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_5(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_7(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_6(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_8(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_7(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_9(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_8(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_10(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_9(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_11(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_10(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_12(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_11(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_13(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_12(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_14(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_13(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_15(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_14(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_16(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_15(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_17(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_16(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_18(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_17(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_19(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_18(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_20(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_19(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_21(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_20(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_22(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_21(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_23(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_22(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_24(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_23(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_25(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_24(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_26(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_25(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_27(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_26(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_28(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_27(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_29(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_28(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_30(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_29(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_31(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_30(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_32(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_31(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_33(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_32(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_34(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_33(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_35(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_34(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_36(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_35(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_37(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_36(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_38(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_37(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_39(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_38(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_40(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_39(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_41(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_40(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_42(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_41(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_43(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_42(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_44(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_43(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_45(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_44(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_46(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_45(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_47(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_46(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_48(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_47(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_49(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_48(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_50(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_49(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_51(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_50(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_52(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_51(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_53(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_52(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_54(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_53(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_55(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_54(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_56(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_55(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_57(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_56(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_58(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_57(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_59(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_58(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_60(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_59(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_61(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_60(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_62(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_61(func, i + 1, __VA_ARGS__)
#define _RFEL_REPEAT_63(func, i, arg, ...) func(i, arg) _RFEL_REPEAT_62(func, i + 1, __VA_ARGS__)

#define _REFL_STR(x) #x
#define _REFL_STRING(x) _REFL_STR(x)
#define _REFL_CONCATE(x, y) x##y
#define _REFL_PASTE(x, y) _REFL_CONCATE(x, y)

#define _REFL_PARE(...) __VA_ARGS__
#define _REFL_EAT(...)
#define _REFL_PAIR(x) _REFL_PARE x
#define _REFL_STRIP(x) _REFL_EAT x

#endif