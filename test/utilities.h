#ifndef CBOR_UTILITIES_H
#define CBOR_UTILITIES_H

#include <string>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>

void write_date_time(char *buff, size_t buff_size)
{
    auto now = std::chrono::high_resolution_clock::now();
    long long ms_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
    tm time_info;
    localtime_r(&sec_since_epoch, &time_info);
    snprintf(buff, buff_size, "%04d%02d%02d_%02d%02d%02d.%03lld",
             1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
             time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
}

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
            fprintf(stderr, "bad idx:%lld,orig:%d,decoded:%d\n", i, orig, v);
        }
    }
    return bad;
}

std::unique_ptr<uint8_t> readhfile(const char *filename, int64_t &len)
{
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::unique_ptr<uint8_t> buf;
    buf.reset(new uint8_t[len]);
    fread(buf.get(), 1, len, f);
    return buf;
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