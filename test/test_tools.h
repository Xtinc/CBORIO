#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H

#include <random>
#include <iostream>
#include <iomanip>
#include <chrono>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

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
    Timer() : time_(std::chrono::system_clock::now()) {}
    double elapsed() const { return std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - time_).count(); }
    std::chrono::system_clock::time_point time_;
};

int checkBytes(uint8_t *buf, uint8_t *actual, int64_t len)
{
    int bad = 0;
    for (int64_t i = 0; i < len; ++i)
    {
        uint8_t orig = actual[i];
        uint8_t v = buf[i];
        if (orig != v)
        {
            if (++bad > 20)
            {
                fprintf(stderr, "more than 20 busted\n");
                break;
            }
            
            //fprintf(stderr, "bad idx:%lld,orig:%d,decoded:%d\n", i, orig, v);
        }
    }
    return bad;
}

template <typename T>
void generate_rd(std::vector<T> &ar)
{

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dis(0.0, 100000.0);
    for (size_t i = 0; i < ar.size(); ++i)
    {
        ar[i] = dis(gen);
    }
}

#endif