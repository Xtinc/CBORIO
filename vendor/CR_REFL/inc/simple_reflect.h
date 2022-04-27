#ifndef SIMPLE_REFLECT_H
#define SIMPLE_REFLECT_H
#include "simple_macro.h"
#include <iostream>
#include <utility>

// reference:
// http://pfultz2.com/blog/2012/07/31/reflection-in-under-100-lines
// https://zhuanlan.zhihu.com/p/27186999

#if __cplusplus < 201703L
namespace
{
    template <typename... T>
    struct make_void
    {
        using type = void;
    };
    template <typename... T>
    using void_t = typename make_void<T...>::type;
}
#else
template <typename T>
using void_t = std::void_t<T>;
#endif

#if __cplusplus < 201402L
namespace
{
    // some functions in STL

    template <class T, T... Ints>
    class integer_sequence
    {
    public:
        using value_type = T;
        static_assert(std::is_integral<value_type>::value,
                      "not integral type");
        static constexpr std::size_t size() noexcept
        {
            return sizeof...(Ints);
        }
    };
    template <std::size_t... Inds>
    using index_sequence = integer_sequence<std::size_t, Inds...>;

    namespace Detail_
    {

        template <class T, T Begin, T End, bool>
        struct IntSeqImpl
        {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value,
                          "not integral type");
            static_assert(Begin >= 0 && Begin < End,
                          "unexpected argument (Begin<0 || Begin<=End)");

            template <class, class>
            struct IntSeqCombiner;

            template <TValue... Inds0, TValue... Inds1>
            struct IntSeqCombiner<integer_sequence<TValue, Inds0...>,
                                  integer_sequence<TValue, Inds1...>>
            {
                using TResult = integer_sequence<TValue, Inds0..., Inds1...>;
            };

            using TResult = typename IntSeqCombiner<typename IntSeqImpl<TValue, Begin, Begin + (End - Begin) / 2,
                                                                        (End - Begin) / 2 == 1>::TResult,
                                                    typename IntSeqImpl<TValue, Begin + (End - Begin) / 2, End,
                                                                        (End - Begin + 1) / 2 == 1>::TResult>::TResult;
        };

        template <class T, T Begin>
        struct IntSeqImpl<T, Begin, Begin, false>
        {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value,
                          "not integral type");
            static_assert(Begin >= 0, "unexpected argument (Begin<0)");
            using TResult = integer_sequence<TValue>;
        };

        template <class T, T Begin, T End>
        struct IntSeqImpl<T, Begin, End, true>
        {
            using TValue = T;
            static_assert(std::is_integral<TValue>::value,
                          "not integral type");
            static_assert(Begin >= 0, "unexpected argument (Begin<0)");
            using TResult = integer_sequence<TValue, Begin>;
        };
    }

    template <class T, T N>
    using make_integer_sequence =
        typename Detail_::IntSeqImpl<T, 0, N, (N - 0) == 1>::TResult;

    template <std::size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;

    template <class... T>
    using index_sequence_for = make_index_sequence<sizeof...(T)>;
}
#else
template <std::size_t... Inds>
using index_sequence = std::integer_sequence<std::size_t, Inds...>;

template <class T, T N>
using make_integer_sequence = std::make_integer_sequence<T, N>;

template <std::size_t N>
using make_index_sequence = std::make_integer_sequence<std::size_t, N>;

template <class... T>
using index_sequence_for = std::make_index_sequence<sizeof...(T)>;

#endif

#define FIELD_EACH(i, arg)                             \
    _REFL_PAIR(arg);                                   \
    template <typename T>                              \
    struct FIELD<T, i>                                 \
    {                                                  \
        T &obj;                                        \
        FIELD(T &obj) : obj(obj) {}                    \
        auto value() -> decltype(obj._REFL_STRIP(arg)) \
        {                                              \
            return (obj._REFL_STRIP(arg));             \
        }                                              \
        static constexpr const char *name()            \
        {                                              \
            return _REFL_STRING(_REFL_STRIP(arg));     \
        }                                              \
    };

#define DEFINE_STRUCT(st, ...)                                              \
    struct st                                                               \
    {                                                                       \
        template <typename, size_t>                                         \
        struct FIELD;                                                       \
        static constexpr const char *_field_name_ = _REFL_STRING(st);       \
        static constexpr size_t _field_count_ = GET_ARG_COUNT(__VA_ARGS__); \
        _REFL_PASTE(_RFEL_REPEAT_, GET_ARG_COUNT(__VA_ARGS__))              \
        (FIELD_EACH, 0, __VA_ARGS__)                                        \
    }

#define REFL(var) refl::REFLINFO(_REFL_STRING(var), var)

namespace refl
{
    template <typename T>
    struct refl_info_st
    {
        const char *st_name;
        T st_t;
    };

    template <typename>
    struct is_refl_info_st : std::false_type
    {
    };

    template <typename T>
    struct is_refl_info_st<refl_info_st<T>> : std::true_type
    {
    };

    template <typename T, typename = void>
    struct IsReflected : std::false_type
    {
    };

    template <typename T>
    struct IsReflected<T, void_t<decltype(&T::_field_count_)>>
        : std::true_type
    {
    };

    template <typename T, typename F, size_t... Is>
    inline constexpr void forEach(T &&obj, F &&f, index_sequence<Is...>)
    {
        using TDECAY = typename std::decay<T>::type;
        // tricky
        (void)std::initializer_list<size_t>{
            (f(typename TDECAY::template FIELD<T, Is>(std::forward<T>(obj)).name(),
               typename TDECAY::template FIELD<T, Is>(std::forward<T>(obj)).value()),
             Is)...};
    }

    template <typename T, typename F>
    inline constexpr void forEach(T &&obj, F &&f)
    {
        using TP = typename std::decay<T>::type;
        forEach(std::forward<T>(obj),
                std::forward<F>(f),
                make_index_sequence<TP::_field_count_>{});
    }

    template <typename T>
    inline constexpr auto REFLINFO(const char *name, T &t)
        -> refl_info_st<T>
    {
        refl_info_st<T> st = {name, t};
        return st;
    }

    struct GFunc_out;
    struct GFunc_in;

    template <typename T>
    typename std::enable_if<!IsReflected<T>::value>::type
    serializeObj(std::ostream &out, T obj,
                 const char *fieldName = "", int depth = 0)
    {
        (void)depth;
        out << fieldName << ':' << obj << std::endl;
    }

    template <typename T>
    typename std::enable_if<IsReflected<T>::value>::type
    serializeObj(std::ostream &out, T obj,
                 const char *fieldName = "", int depth = 0)
    {
        out << fieldName << (*fieldName ? ": {" : "{") << std::endl;
        forEach(obj, GFunc_out(out, depth));
        out << "}" << std::endl;
    }

    struct GFunc_out
    {
    private:
        std::ostream &out;
        int depth;

    public:
        GFunc_out(std::ostream &_os, int _depth)
            : out(_os), depth(_depth)
        {
        }
        template <typename Field, typename Name>
        void operator()(Field &&field, Name &&name)
        {
            serializeObj(out, name, field, depth + 1);
        }
    };

    template <typename T>
    typename std::enable_if<IsReflected<T>::value>::type
    deserializeObj(std::istream &in, T &obj,
                   const char *fieldName = "")
    {
        std::string token;
        in >> token; // eat '{'
        if (*fieldName)
        {
            in >> token;
        }

        forEach(obj, GFunc_in(in));

        in >> token; // eat '}'
    }

    template <typename T>
    typename std::enable_if<!IsReflected<T>::value>::type
    deserializeObj(std::istream &in, T &obj,
                   const char *fieldName = "")
    {
        if (*fieldName)
        {
            std::string token;
            in >> token;
        }
        in >> obj; // dump value
    }

    struct GFunc_in
    {
    private:
        std::istream &in;

    public:
        GFunc_in(std::istream &_is)
            : in(_is)
        {
        }
        template <typename Field, typename Name>
        void operator()(Field &&field, Name &&name)
        {
            deserializeObj(in, name, field);
        }
    };
}

#endif