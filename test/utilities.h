#ifndef CBOR_UTILITIES_H
#define CBOR_UTILITIES_H

#include <string>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <iostream>
#include <iomanip>
#include <chrono>

template <class T>
std::string type_name()
{
    typedef typename std::remove_reference<T>::type TR;
    std::unique_ptr<char, void (*)(void *)> own(
#ifndef _MSC_VER
        abi::__cxa_demangle(typeid(TR).name(), nullptr,
                            nullptr, nullptr),
#else
        nullptr,
#endif
        std::free);
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}

struct HexCharStruct
{
    unsigned char c;
    HexCharStruct(unsigned char _c) : c(_c) {}
};

inline std::ostream &operator<<(std::ostream &o, const HexCharStruct &hs)
{
    return (o << std::setw(2) << std::setfill('0') << std::hex << (int)hs.c);
}

inline HexCharStruct hex(unsigned char _c)
{
    return HexCharStruct(_c);
}

// http://mattmahoney.net/dc/textdata.html
struct Timer
{
    Timer() : time_(std::chrono::high_resolution_clock::now()) {}
    double elapsed() const { return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - time_).count(); }
    std::chrono::high_resolution_clock::time_point time_;
};

#endif