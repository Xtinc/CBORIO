#include "my_class.h"
#include "gtest/gtest.h"
#include "CR_REFL/simple_reflect.h"
#include <random>

#define RO_DECODER_CLS fl.clear();
#define RO_DECODER_RUN                          \
    do                                          \
    {                                           \
        ro_file ro(fl.getData(), fl.getSize()); \
        cborio::decoder de(ro, hd);             \
        de.run();                               \
    } while (0);

template <typename T>
void generate_rd(std::vector<T> &ar)
{

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<T> dis(0.0, 100000.0);
    for (auto i = 0; i < ar.size(); ++i)
    {
        ar[i] = dis(gen);
    }
}
class CBOR_O_TestCase : public ::testing::Test
{
public:
    CBOR_O_TestCase() : en(ios)
    {
    }
    my_o ios;
    cborio::encoder en;
};

class CBOR_I_TestCase : public ::testing::Test
{
public:
    CBOR_I_TestCase() : fl(100), en(fl)
    {
    }
    wo_file fl;
    hd_debug hd;
    cborio::encoder en;
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(CBOR_O_TestCase, signed_short)
{
    for (short i : {2, 14, 25, 56, 241, -21, -124, -5, -5116, -24901})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x02,0x0e,0x1819,0x1838,0x18f1,0x34,0x387B,0x24,0x3913FB,0x396144
}

TEST_F(CBOR_O_TestCase, signed_int)
{

    for (auto i : {100, 1000, 10000, 100000, -100, -100000, -87923000})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x1864,0x193e8,0x192710,0x1a0186a0,0x3863,0x3A0001869F,0x3A053D9937
}

TEST_F(CBOR_O_TestCase, signed_ll)
{

    for (auto i : {3000000000, 452384728947, 17515481548154, 435678399658346583, -274632784628453285})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0x1AB2D05E00,0x1B00000069543B2773,0x1B00000FEE240E457A,0x1B060BD73237F24857,0x3B03CFB11003748FA4
}

TEST_F(CBOR_O_TestCase, float_num)
{

    for (auto i : {0.0754f, 34.12f, 7.986f, -46583.46f, -2742.85f})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0xfa3d9a6b51,0xFA42087AE1,0xFA40FF8D50,0xFAC735F776,0xFAC52B6D9A
}

TEST_F(CBOR_O_TestCase, double_num)
{

    for (auto i : {0.000754, 34.12, 7.98646471, 4356783996583.46583, -27463278462.8453285})
    {
        en.write_data(i);
        std::cout << std::endl;
    }
    // 0xFB3F48B502ABABEAD5,0xFB40410F5C28F5C28F,0xFB401FF223CE106EB8,0xFB428FB3247FF53BBA,0xFBC21993C17DFB619E
}

TEST_F(CBOR_O_TestCase, stl_string)
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

TEST_F(CBOR_O_TestCase, char_array)
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

TEST_F(CBOR_O_TestCase, stl_list)
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

TEST_F(CBOR_O_TestCase, stl_map)
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
    // A3657465737431831A000BF190187B3A00053CC9657465737432831A00017EB1395B7F393048657465737433831904BC3903DC00
}

TEST_F(CBOR_I_TestCase, signed_short)
{
    RO_DECODER_CLS
    for (short i : {2, 14, 25, 56, 241, -21, -124, -5, -5116, -24901})
    {
        en.write_data(i);
    }
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, signed_int)
{
    RO_DECODER_CLS
    for (auto i : {100, 1000, 10000, 100000, -100, -100000, -87923000})
    {
        en.write_data(i);
    }
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, signed_ll)
{
    RO_DECODER_CLS
    for (auto i : {3000000000, 452384728947, 17515481548154, 435678399658346583, -274632784628453285})
    {
        en.write_data(i);
    }
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, float_num)
{
    RO_DECODER_CLS
    for (auto i : {0.0754f, 34.12f, 7.986f, -46583.46f, -2742.85f})
    {
        en.write_data(i);
    }
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, double_num)
{
    RO_DECODER_CLS
    for (auto i : {0.000754, 34.12, 7.98646471, 4356783996583.46583, -27463278462.8453285})
    {
        en.write_data(i);
    }
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, stl_string)
{
    RO_DECODER_CLS
    for (std::string i : {"0.000754", "3ad4f12", "bhdsf", "0xashdgox", ""})
    {
        en.write_data(i);
    }
    en.write_data(std::string("lvaue"));
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, char_array)
{
    RO_DECODER_CLS
    for (auto i : {"0.000754", "3ad4f12", "bhdsf", "0xashdgox", ""})
    {
        en.write_data(i, sizeof(i));
    }
    char p[10] = "werttt";
    en.write_data(p, strlen(p) + 1);
    std::string str("ceshisdf");
    en.write_data(str.c_str(), str.length());
    en.write_data("lvaue", 5);
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, stl_list)
{
    RO_DECODER_CLS
    std::vector<int> ls1 = {1, 2, 3, 4, 5};
    en.write_data(ls1);
    en.write_data(std::list<double>(3, 5.056));
    std::deque<std::string> qu = {"cehi", "32846de", "queudbvf", "%^45243**&/n"};
    en.write_data(qu);
    en.write_data(std::vector<char>{'a', 'b', 'c', 'd'});
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, stl_map)
{
    RO_DECODER_CLS
    std::map<int, int> mp1 = {{1, 2}, {2, 2}, {3, 56}};
    en.write_data(mp1);
    std::unordered_map<std::string, std::string> mp2 = {{"key", "value"}, {"jkfdh", "vfd"}, {"c876rw%^", ""}};
    const std::unordered_map<std::string, std::string> &pp = mp2;
    en.write_data(pp);
    std::vector<int> a1 = {782736, 123, -343242};
    std::vector<int> a2 = {97969, -23424, -12361};
    std::vector<int> a3 = {1212, -989, 0};
    std::map<std::string, std::vector<int>> mp3;
    mp3.insert(std::make_pair("test1", a1));
    mp3.insert(std::make_pair("test2", a2));
    mp3.insert(std::make_pair("test3", a3));
    en.write_data(mp3);
    RO_DECODER_RUN
}

TEST_F(CBOR_I_TestCase, stream_input)
{
    RO_DECODER_CLS
    en << std::list<int>(10, 1919);
    RO_DECODER_RUN
}

TEST(CBOR_IO_TestCase, write_diskfile_str)
{
    std::ofstream ou("s.txt", std::ios::out);
    wo_disk_file woo("s.cbor");
    cborio::encoder en(woo);
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dis_len(0, 1000);
    std::uniform_int_distribution<int> dis_char{'a', 'z'};
    for (int i = 0; i < 10000; ++i)
    {
        std::string str(dis_len(gen), '\0');
        for (auto &i : str)
        {
            i = dis_char(gen);
        }
        ou << "s" << str << '\n';
        en << str;
    }
}

TEST(CBOR_IO_TestCase, write_diskfile_float)
{
    std::vector<double> data(10000, 0);
    generate_rd(data);
    std::ofstream ou("d.txt", std::ios::out);
    wo_disk_file woo("d.cbor");
    cborio::encoder en(woo);
    for (auto &i : data)
    {
        ou << "f" << i << " ";
        en << i;
    }
}

TEST(CBOR_IO_TestCase, write_diskfile_int)
{
    std::vector<int> data(10000, 0);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 100000);
    for (auto &i : data)
    {
        i = dis(gen);
    }
    std::ofstream ou("i.txt", std::ios::out);
    wo_disk_file woo("i.cbor");
    cborio::encoder en(woo);
    for (auto &i : data)
    {
        ou << "i" << i << " ";
        en << i;
    }
}

TEST(CBOR_IO_TestCase, read_diskfile)
{
    ro_disk_file roo("s.cbor");
    if (roo.is_open())
    {
        hd_debug hdb;
        cborio::decoder de(roo, hdb);
        de.run();
        roo.close();
    }
}

DEFINE_STRUCT(Point,
              (double)x,
              (double)y);

DEFINE_STRUCT(Rect,
              (Point)p1,
              (Point)p2,
              (uint32_t)color);

TEST(REFL_TEST, refk)
{
    std::stringstream result; // serialize result
    {
        Rect rect{
            {1.2, 3.4},
            {5.6, 7.8},
            12345678,
        };
        serializeObj(result, rect);
    }
    std::cout << "serialize rect result:" << std::endl
              << result.str() << std::endl;

    Rect rect2;
    deserializeObj(result, rect2);
    std::cout << "deserialize rect result:" << std::endl;
    serializeObj(std::cout, rect2);
}