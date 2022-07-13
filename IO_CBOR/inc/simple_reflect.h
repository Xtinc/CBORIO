#ifndef SIMPLE_REFELECT_H
#define SIMPLE_REFELECT_H

#include <tuple>
#include <functional>
#include <typeinfo>

using REFL_HANDLER = std::function<void(const char *fieldname, int recursive_depth)>;

template <typename T>
struct StructMetaInfo
{
    static std::tuple<> Info()
    {
        return std::make_tuple();
    }
};

#define REFL(Struct, ...)                                    \
    template <>                                              \
    struct StructMetaInfo<Struct>                            \
    {                                                        \
        static decltype(std::make_tuple(__VA_ARGS__)) Info() \
        {                                                    \
            return std::make_tuple(__VA_ARGS__);             \
        }                                                    \
    };

#define FIELD(class, field) \
    std::make_tuple(#field, &class ::field)

template <typename T, typename Fields, typename F, size_t... Is>
void refl_foreach(T &&obj, Fields &&fields, F &&f, std::index_sequence<Is...>)
{
    (void)std::initializer_list<size_t>{(f(
                                             std::get<0>(std::get<Is>(fields)),
                                             obj.*std::get<1>(std::get<Is>(fields))),
                                         Is)...};
}

template <typename T, typename F>
void refl_foreach(T &&obj, F &&f)
{
    auto fields = StructMetaInfo<std::decay_t<T>>::Info();
    refl_foreach(std::forward<T>(obj),
                 fields,
                 std::forward<F>(f),
                 std::make_index_sequence<std::tuple_size<decltype(fields)>::value>{});
}

template <typename F>
struct refl_recur_func;

template <typename T, typename F, std::enable_if_t<std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth)
{
    f(fieldName, depth);
    refl_foreach(obj, refl_recur_func<F>(f, depth));
}

template <typename T, typename F, std::enable_if_t<!std::is_class<std::decay_t<T>>::value> * = nullptr>
void refl_recur_obj(T &&obj, F &&f, const char *fieldName, int depth)
{
    f(fieldName, depth);
}

template <typename F>
struct refl_recur_func
{
public:
    refl_recur_func(const F &_f, int _depth) : f(_f), depth(_depth) {}

    template <typename Value>
    void operator()(const char *fieldName, Value &&value)
    {
        refl_recur_obj(value, f, fieldName, depth + 1);
    }

private:
    F f;
    int depth;
};

#endif