#ifndef SIMPLE_REFELECT_H
#define SIMPLE_REFELECT_H

#include <tuple>
#include <functional>
#include <typeinfo>

using REFL_HANDLER = std::function<void(const char *fieldname, int recursive_depth)>;

template <typename T>
inline constexpr auto StructMetaInfo()
{
    return std::make_tuple();
}

#define REFL(Struct, ...)                          \
    template <>                                    \
    inline constexpr auto StructMetaInfo<Struct>() \
    {                                              \
        using T = Struct;                          \
        return std::make_tuple(                    \
            __VA_ARGS__);                          \
    };

#define FIELD(field) \
    std::make_tuple(#field, &T::field)

template <typename T, typename Fields, typename F, size_t... Is>
inline constexpr void refl_foreach(T &&obj, Fields &&fields, F &&f, std::index_sequence<Is...>)
{
    auto _ = {(f(
                   std::get<0>(std::get<Is>(fields)),
                   obj.*std::get<1>(std::get<Is>(fields))),
               Is)...,
              0ul};
}

template <typename T, typename F>
inline constexpr void refl_foreach(T &&obj, F &&f)
{
    constexpr auto fields = StructMetaInfo<std::decay_t<T>>();
    refl_foreach (std::forward<T>(obj),
             fields,
             std::forward<F>(f),
             std::make_index_sequence<std::tuple_size<decltype(fields)>::value>{})
        ;
}

template <typename T, typename F, std::enable_if_t<std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth)
{
    f(fieldName, depth);
    refl_foreach (obj, [depth, f](auto &&fieldName, auto &&value)
             { refl_recur_obj(value, f, fieldName, depth + 1); })
        ;
}

template <typename T, typename F, std::enable_if_t<!std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth)
{
    f(fieldName, depth);
}
#endif