#include "IO_CBOR/inc/encoder.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>
#include <map>
#include <unordered_map>

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

class my_io : public cborio::output
{
private:
public:
    my_io() {}
    void put_byte(unsigned char c) override
    {
        std::cout << hex(c);
        return;
    }
    void put_bytes(const unsigned char *data, int size) override
    {
        for (int i = 0; i < size; ++i)
        {
            std::cout << hex(*(data + i));
        }
        return;
    }
};

class CBOR_IO_TestCase : public ::testing::Test
{
public:
    CBOR_IO_TestCase() : en(ios)
    {
    }
    my_io ios;
    cborio::encoder en;
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(CBOR_IO_TestCase, signed_short)
{
    for (short i : {2, 14, 25, 56, 241, -21, -124, -5, -5116, -24901})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x02,0x0e,0x1819,0x1838,0x18f1,0x34,0x387B,0x24,0x3913FB,0x396144
}

TEST_F(CBOR_IO_TestCase, unsigned_int)
{

    for (auto i : {100, 1000, 10000, 100000, -100, -100000, -87923000})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x1864,0x193e8,0x192710,0x1a0186a0,0x3863,0x3A0001869F,0x3A053D9937
}

TEST_F(CBOR_IO_TestCase, unsigned_ll)
{

    for (auto i : {3000000000, 452384728947, 17515481548154, 435678399658346583, -274632784628453285})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x1AB2D05E00,0x1B00000069543B2773,0x1B00000FEE240E457A,0x1B060BD73237F24857,0x3B03CFB11003748FA4
}

TEST_F(CBOR_IO_TestCase, float_num)
{

    for (auto i : {0.0754f, 34.12f, 7.986f, -46583.46f, -2742.85f})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0xfa3d9a6b51,0xFA42087AE1,0xFA40FF8D50,0xFAC735F776,0xFAC52B6D9A
}

TEST_F(CBOR_IO_TestCase, double_num)
{

    for (auto i : {0.000754, 34.12, 7.98646471, 4356783996583.46583, -27463278462.8453285})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0xFB3F48B502ABABEAD5,0xFB40410F5C28F5C28F,0xFB401FF223CE106EB8,0xFB428FB3247FF53BBA,0xFBC21993C17DFB619E
}

TEST_F(CBOR_IO_TestCase, stl_string)
{

    for (std::string i : {"0.000754", "3ad4f12", "bhdsf", "0xashdgox", ""})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    en.write_data(std::string("lvaue"));
    std::cout << std::endl;
    // 0x68302E303030373534
    // 0x6733616434663132
    // 0x656268647366
    // 0x69307861736864676F78
    // 0x60
    // 0x656c76617565
}

TEST_F(CBOR_IO_TestCase, char_array)
{

    for (auto i : {"0.000754", "3ad4f12", "bhdsf", "0xashdgox", ""})
    {
        en.write_data(i, sizeof(i));
        std::cout << std::endl;
    }
    char p[10] = "werttt";
    en.write_data(p, strlen(p) + 1);
    std::cout << std::endl;
    std::string str("ceshisdf");
    en.write_data(str.c_str(), str.length());
    std::cout << std::endl;
    en.write_data("lvaue", 5);
    std::cout << std::endl;
    // 0xFB3F48B502ABABEAD5,0xFB40410F5C28F5C28F,0xFB401FF223CE106EB8,0xFB428FB3247FF53BBA,0xFBC21993C17DFB619E
    // 0x66776572747474
    // 0x686365736869736466
    // 0x656c76617565
}

TEST_F(CBOR_IO_TestCase, stl_list)
{
    std::vector<int> ls1 = {1, 2, 3, 4, 5};
    en.write_data(ls1);
    // 0x850102030405
    std::cout << std::endl;
    en.write_data(std::list<double>(3, 5.056));
    std::cout << std::endl;
    // 0x83FB4014395810624DD3FB4014395810624DD3FB4014395810624DD3
    std::deque<std::string> qu = {"cehi", "32846de", "queudbvf", "%^45243**&/n"};
    en.write_data(qu);
    std::cout << std::endl;
    // 0x84646365686967333238343664656871756575646276666C255E34353234332A2A262F6E
    en.write_data(std::vector<char>{'a', 'b', 'c', 'd'});
    std::cout << std::endl;
    // 0x841861186218631864
}
TEST_F(CBOR_IO_TestCase, stl_map)
{
    std::map<int, int> mp1 = {{1, 2}, {2, 2}, {3, 56}};
    en.write_data(mp1);
    std::cout << std::endl;
    // 0xA301020202031838
    std::unordered_map<std::string, std::string> mp2 = {{"key", "value"}, {"jkfdh", "vfd"}, {"c876rw%^", ""}};
    const std::unordered_map<std::string, std::string> &pp = mp2;
    en.write_data(pp);
    std::cout << std::endl;
    // 没有顺序，每次都需要在网站上看https://cbor.me/
    std::vector<int> a1 = {782736, 123, -343242};
    std::vector<int> a2 = {97969, -23424, -12361};
    std::vector<int> a3 = {1212, -989, 0};
    std::map<std::string, std::vector<int>> mp3;
    mp3.insert(std::make_pair("test1", a1));
    mp3.insert(std::make_pair("test2", a2));
    mp3.insert(std::make_pair("test3", a3));
    en.write_data(mp3);
    std::cout << std::endl;
    //A3657465737431831A000BF190187B3A00053CC9657465737432831A00017EB1395B7F393048657465737433831904BC3903DC00          
}