#ifndef CBOR_TEMPLATES_H
#define CBOR_TEMPLATES_H

#include <type_traits>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cborio
{
    namespace
    {
        template <typename T>
        struct is_stl_list : std::false_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::vector<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::deque<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::list<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::forward_list<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::stack<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::queue<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_list<std::priority_queue<Args...>> : std::true_type
        {
        };

        template <typename T>
        struct is_stl_map : std::false_type
        {
        };
        template <typename... Args>
        struct is_stl_map<std::map<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_map<std::multimap<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_map<std::unordered_map<Args...>> : std::true_type
        {
        };
        template <typename... Args>
        struct is_stl_map<std::unordered_multimap<Args...>> : std::true_type
        {
        };
    }

    template <typename T>
    struct ISTList
    {
        static constexpr bool const value = is_stl_list<typename std::decay<T>::type>::value;
    };
    template <typename T>
    struct ISTLmap
    {
        static constexpr bool const value = is_stl_map<typename std::decay<T>::type>::value;
    };
    template <typename T>
    struct ISTRing
    {
        static constexpr bool const value = std::is_same<typename std::decay<T>::type, std::string>::value;
    };

    template <typename T>
    struct CharDispatch
    {
        using Tag = void *;
    };
    template <>
    struct CharDispatch<unsigned char *>
    {
        using Tag = unsigned char *;
    };
    template <>
    struct CharDispatch<const unsigned char *>
    {
        using Tag = unsigned char *;
    };
    template <>
    struct CharDispatch<char *>
    {
        using Tag = char *;
    };
    template <>
    struct CharDispatch<const char *>
    {
        using Tag = char *;
    };

    template <typename T>
    struct is_unsigned : public std::false_type
    {
    };
    template <>
    struct is_unsigned<unsigned short> : public std::true_type
    {
    };
    template <>
    struct is_unsigned<unsigned int> : public std::true_type
    {
    };
    template <>
    struct is_unsigned<unsigned long> : public std::true_type
    {
    };
    template <>
    struct is_unsigned<unsigned long long> : public std::true_type
    {
    };

    template <typename T>
    struct is_signed : public std::false_type
    {
    };
    template <>
    struct is_signed<short> : public std::true_type
    {
    };
    template <>
    struct is_signed<int> : public std::true_type
    {
    };
    template <>
    struct is_signed<long> : public std::true_type
    {
    };
    template <>
    struct is_signed<long long> : public std::true_type
    {
    };

    template <typename T>
    struct is_boolean : public std::false_type
    {
    };
    template <>
    struct is_boolean<bool> : public std::true_type
    {
    };

    template <typename T>
    struct is_char : public std::false_type
    {
    };
    template <>
    struct is_char<char> : public std::true_type
    {
    };
    template <>
    struct is_char<unsigned char> : public std::true_type
    {
    };

    template <typename T>
    struct is_charptr : public std::false_type
    {
    };
    template <>
    struct is_charptr<char *> : public std::true_type
    {
    };
    template <>//todo char* should be str or unsigned?
    struct is_charptr<const char *> : public std::true_type
    {
    };
}
#endif