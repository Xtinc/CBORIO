#include "gtest/gtest.h"
#include "reclog.h"
#include "my_class.h"
#include "reclog_impl.h"
#include "test_tools.h"
#include <thread>

DEFINE_STRUCT(Point,
              (double)x,
              (double)y);
DEFINE_STRUCT(Rect,
              (Point)p1,
              (Point)p2,
              (uint32_t)color);
DEFINE_STRUCT(STRWNUM,
              (std::string)str_a,
              (double)num_b);

DEFINE_STRUCT(TEST_CBOR,
              (int)a,
              (double)b);

void generate_rnd_str(std::vector<STRWNUM> &strlist, size_t &cnt)
{
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dis_len(0, 1000);
    std::uniform_int_distribution<int> dis_char{'a', 'z'};
    std::uniform_real_distribution<double> dis_flt(-10000, 1000000);

    STRWNUM stw;
    cnt = 0;
    for (int i = 0; i < 10000; ++i)
    {
        std::string str(dis_len(gen), '\0');
        for (auto &j : str)
        {
            j = static_cast<char>(dis_char(gen));
        }
        double d = dis_flt(gen);
        stw = {str, d};
        strlist.push_back(stw);
        cnt = cnt + str.size() + 8;
    }
}

class RECLOG_TestCase : public ::testing::Test
{
public:
    RECLOG_TestCase()
    {
        RECLOG::RECONFIG::InitREC();
        generate_rnd_str(strlist, cnt);
    }
    std::vector<STRWNUM> strlist;
    size_t cnt;
};

class RECFILE_TestCase : public ::testing::Test
{
public:
    RECFILE_TestCase()
    {
        RECLOG::RECONFIG::InitREC("st.cbor", true);
        generate_rnd_str(strlist, cnt);
    }
    std::vector<STRWNUM> strlist;
    size_t cnt;
    ~RECFILE_TestCase()
    {
    }
};

class RECRAW_TestCase : public ::testing::Test
{
public:
    RECRAW_TestCase()
    {
        RECLOG::RECONFIG::InitREC("st.raw");
        generate_rnd_str(strlist, cnt);
    }
    std::vector<STRWNUM> strlist;
    size_t cnt;
    ~RECRAW_TestCase()
    {
    }
};

struct test_encoder_stream_io
{
    int a;
    int b;
    double c;
    std::string d;

    test_encoder_stream_io(int _a, int _b, double _c, std::string _d) : a(_a), b(_b), c(_c), d(_d) {}

    friend std::ostream &operator<<(std::ostream &os, const test_encoder_stream_io &ss)
    {
        os << ss.a << ss.b << ss.c << ss.d;
        return os;
    };
};

void test_print_speed(const std::vector<STRWNUM> &strlist, const size_t &cnt, std::function<void(const STRWNUM &)> &&f)
{
    Timer timer;
    for (auto &i : strlist)
    {
        f(i);
    }
    double elapsed = timer.elapsed() / 10.0;
    printf("%.2lf usecond per log.\n", elapsed);
    elapsed = elapsed / 100.0;
    printf("Print %7.4f kbytes,in %.2lf seconds, %.2lf MB/s\n",
           cnt / 1000.0, elapsed, (cnt / (1024. * 1024.)) / elapsed);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(RECLOG_TestCase, sio_time)
{
    char *td = new char[50];
    for (int i = 0; i < 100; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        print_date_time(get_date_time(), td, 50);
        RECLOG(log) << td[i];
    }

    delete[] td;
}

TEST_F(RECLOG_TestCase, sio_thread)
{
    char *td = new char[50];
    print_thread_name(get_thread_name(), td, 50);
    RECLOG(log) << td;
    delete[] td;
}

TEST_F(RECLOG_TestCase, log_format)
{
    float lots = 3.141592f;
    float little1 = 2.25;
    float little2 = 1.5;
    float whole = 4.00000;

    RECLOG(log) << "Some values with noshowpoint (the default)";

    RECLOG(log) << "lots:    " << lots;
    RECLOG(log) << "little1: " << little1;
    RECLOG(log) << "little2: " << little2;
    RECLOG(log) << "whole:   " << whole;

    RECLOG(log) << "The same values with showpoint";

    RECLOG(log) << "lots:    " << std::showpoint << lots;
    RECLOG(log) << "little1: " << std::showpoint << little1;
    RECLOG(log) << "little2: " << std::showpoint << little2;
    RECLOG(log) << "whole:   " << std::showpoint << whole;
}

TEST_F(RECLOG_TestCase, sio_func)
{
    TEST_CBOR tcb = {1, 8.9};
    uint64_t ces = 887;
    test_encoder_stream_io tesi(1, 1, 2.5, "opeassds");
    Timer timer;
    for (int i = 0; i < 10000; ++i)
    {
        RECLOG(log) << tcb << "cessjo" << 1 << 5.599 << -1 << ces << tcb << tesi;
    }
    double elapsed = timer.elapsed();
    printf("%.2lf usecond per log.\n", elapsed / 10.0);
}

TEST_F(RECLOG_TestCase, sio_speed)
{
    test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                     { RECLOG(log) << stw; });
}

TEST_F(RECLOG_TestCase, sio_speed_md)
{
    std::vector<std::thread> thdvec;
    for (size_t i = 0; i < 5; ++i)
    {
        thdvec.emplace_back([this]()
                            { test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                                               { RECLOG(log) << stw; }); });
    }
    for (size_t i = 0; i < thdvec.size(); ++i)
    {
        thdvec[i].join();
    }
}

TEST_F(RECFILE_TestCase, fio_func)
{
    TEST_CBOR tcb = {1, 8.9};
    uint32_t ces = 887;
    test_encoder_stream_io tesi(1, 1, 2.5, "opeassds");
    Timer timer;
    for (int i = 0; i < 10000; ++i)
    {
        RECLOG(file) << tcb << "cessjo" << 1 << 5.599 << -1 << ces << tcb << tesi;
    }
    double elapsed = timer.elapsed();
    printf("%.2lf usecond per log.\n", elapsed / 10.0);
}

TEST_F(RECFILE_TestCase, fio_speed)
{
    test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                     { RECLOG(file) << stw; });
}

TEST_F(RECFILE_TestCase, fio_speed_md)
{
    std::vector<std::thread> thdvec;
    for (size_t i = 0; i < 4; ++i)
    {
        thdvec.emplace_back([this]()
                            { test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                                               { RECLOG(file) << stw; }); });
    }
    for (size_t i = 0; i < thdvec.size(); ++i)
    {
        thdvec[i].join();
    }
}

TEST_F(RECRAW_TestCase, raw_func)
{
    TEST_CBOR tcb = {1, 8.9};
    uint64_t ces = 887;
    std::string str("cessjo");
    test_encoder_stream_io tesi(1, 1, 2.5, "opeassds");
    Timer timer;
    for (int i = 0; i < 10000; ++i)
    {
        RECLOG(raw) << tcb.a << tcb.b << str << 1 << 5.599
                    << -1 << ces << tcb.a << tcb.b << tesi.a << tesi.b
                    << tesi.c << tesi.d;
    }
    double elapsed = timer.elapsed();
    printf("%.2lf usecond per log.\n", elapsed / 10.0);
}

TEST_F(RECRAW_TestCase, raw_speed)
{
    test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                     { RECLOG(raw) << stw.num_b << stw.str_a; });
}

TEST_F(RECRAW_TestCase, raw_speed_md)
{
    std::vector<std::thread> thdvec;
    for (size_t i = 0; i < 5; ++i)
    {
        thdvec.emplace_back([this]()
                            { test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                                               { RECLOG(raw) << stw.num_b << stw.str_a; }); });
    }
    for (size_t i = 0; i < thdvec.size(); ++i)
    {
        thdvec[i].join();
    }
}

TEST_F(RECRAW_TestCase, test_net)
{
    for (auto i = 0; i < 100; ++i)
    {
        RECLOG(net) << std::string("ceshi");
    }
}

TEST_F(RECRAW_TestCase, test_net_md)
{
    std::vector<std::thread> thdvec;
    for (size_t i = 0; i < 5; ++i)
    {
        thdvec.emplace_back([this]()
                            { test_print_speed(strlist, cnt, [](const STRWNUM &stw)
                                               { RECLOG(net) << stw.num_b << stw.str_a; }); });
    }
    for (size_t i = 0; i < thdvec.size(); ++i)
    {
        thdvec[i].join();
    }
}

TEST(RECDecoder_TestCase, decompress)
{
    RECLOG::RECONFIG::InitREC("crash");
    std::ifstream ifs("st.cbor0.cpr", std::ios_base::binary);
    std::ofstream ofs("st.cbor0", std::ios_base::binary);
    cborio::decompress(ifs, ofs);
    ifs.close();
    ofs.close();
    ro_disk_file roo("st.cbor0");
    if (roo.is_open())
    {
        hd_debug hdb;
        cborio::decoder de(roo, hdb);
        de.run();
        roo.close();
    }
}