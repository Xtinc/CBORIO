#include <iostream>
#include <tuple>
#include <typeinfo>

template <typename T>
inline constexpr auto StructMeta()
{
    return std::make_tuple();
}

#define REFL(Struct, ...)                      \
    template <>                                \
    inline constexpr auto StructMeta<Struct>() \
    {                                          \
        using T = Struct;                      \
        return std::make_tuple(                \
            __VA_ARGS__);                      \
    };

#define FIELD(field) \
    std::make_tuple(#field, &T::field)

template <typename T, typename Fields, typename F, size_t... Is>
inline constexpr void foreach (T &&obj, Fields && fields, F && f, std::index_sequence<Is...>)
{
    auto _ = {(f(
                   std::get<0>(std::get<Is>(fields)),
                   obj.*std::get<1>(std::get<Is>(fields))),
               Is)...,
              0ul};
}

template <typename T, typename F>
inline constexpr void foreach (T &&obj, F && f)
{
    constexpr auto fields = StructMeta<std::decay_t<T>>();
    foreach (std::forward<T>(obj),
             fields,
             std::forward<F>(f),
             std::make_index_sequence<std::tuple_size<decltype(fields)>::value>{})
        ;
}

inline void indent(int depth)
{
    for (int i = 0; i < depth; ++i)
    {
        std::cout << "    ";
    }
    return;
}

template <typename T,
          typename std::enable_if<!std::is_class<typename std::decay<T>::type>::value>::type * = nullptr>
void dumpObj(T &&obj, const char *fieldName = "", int depth = 0)
{
    indent(depth);
    std::cout << fieldName << ": " << obj << "," << std::endl;
}

template <typename T,
          typename std::enable_if<std::is_class<typename std::decay<T>::type>::value>::type * = nullptr>
void dumpObj(T &&obj, const char *fieldName = "", int depth = 0)
{
    std::cout << fieldName << (*fieldName ? ": {" : "{") << std::endl;
    foreach (obj, [depth](auto &&fieldName, auto &&value)
             { dumpObj(value, fieldName, depth + 1); })
        ;
    indent(depth);
    std::cout << "}" << (depth == 0 ? "" : ",") << std::endl;
}
