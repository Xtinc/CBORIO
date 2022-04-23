#include "gtest/gtest.h"
#include "reclog.h"
#include "my_class.h"
#include "utilities.h"
#include "test_tools.h"
#include <thread>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

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

TEST(REFL_TEST, refk_struct)
{
    std::stringstream result;
    {
        Rect rect{
            {1.2, 3.4},
            {5.6, 7.8},
            12345678,
        };
        result << rect;
    }
    std::cout << "serialize rect result:" << std::endl
              << result.str() << std::endl;

    Rect rect2;
    refl::deserializeObj(result, rect2);
    std::cout << "deserialize rect result:" << std::endl;
    refl::serializeObj(std::cout, rect2);
}

DEFINE_STRUCT(TEST_CBOR,
              (int)a,
              (double)b);

TEST(BAGREC, sio_time)
{
    char *td = new char[50];
    for (int i = 0; i < 100; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        print_date_time(get_date_time(), td, 50);
        std::cout << td << std::endl;
    }

    delete[] td;
}

TEST(BAGREC, sio_thread)
{
    char *td = new char[50];
    print_thread_name(get_thread_name(), td, 50);
    std::cout << td << std::endl;
    delete[] td;
}

TEST(BAGREC, preamble)
{
    print_header();
    char preamble_buffer[REC_PREAMBLE_WIDTH];
    print_preamble(preamble_buffer, sizeof(preamble_buffer), RECLOG::Verbosity_INFO, __FILE__, __LINE__);
    printf("%s", preamble_buffer);
    fflush(stdout);
}

<<<<<<< HEAD
void test_stream_io()
=======
TEST(BAGREC, console_format_print)
>>>>>>> a180e873644545320993f28f175f91b2d862ebe9
{
    RECLOG::INIT_REC();
    float lots = 3.1415926535;
    float little1 = 2.25;
    float little2 = 1.5;
    float whole = 4.00000;

    RECLOG(INFO) << "Some values with noshowpoint (the default)";

    RECLOG(INFO) << "lots:    " << lots;
    RECLOG(INFO) << "little1: " << little1;
    RECLOG(INFO) << "little2: " << little2;
    RECLOG(INFO) << "whole:   " << whole;

    RECLOG(INFO) << "The same values with showpoint";

    RECLOG(INFO) << "lots:    " << std::showpoint << lots;
    RECLOG(INFO) << "little1: " << std::showpoint << little1;
    RECLOG(INFO) << "little2: " << std::showpoint << little2;
    RECLOG(INFO) << "whole:   " << std::showpoint << whole;
}

TEST(BAGREC, stream_io_speed)
{
    RECLOG::INIT_REC();
    TEST_CBOR tcb = {1, 8.9};
    uint64_t ces = 887;
    Timer timer;
    for (int i = 0; i < 10000; ++i)
    {
        RECLOG(INFO) << tcb << "cessjo" << 1 << 5.599 << -1 << ces << tcb;
    }
    double elapsed = timer.elapsed();
    printf("%.2lf usecond\n", elapsed / 10.0);
    fflush(stdout);
}

<<<<<<< HEAD
TEST(BAGREC, stream_io_speed)
{
    test_stream_io();
}

TEST(BAGREC, stream_io_speed_md)
{
    std::thread th1([]()
                    { test_stream_io(); });
    std::thread th2([]()
                    { test_stream_io(); });
    th1.join();
    th2.join();
}

=======
>>>>>>> a180e873644545320993f28f175f91b2d862ebe9
TEST(BAGREC, file_io_speed)
{
    RECLOG::INIT_REC("st.cbor");
    TEST_CBOR tcb = {1, 8.9};
    uint64_t ces = 887;
    Timer timer;
    for (int i = 0; i < 10000; ++i)
    {
        RECLOG(INFO) << tcb << "cessjo" << 1 << 5.599 << -1 << ces << tcb;
    }
    double elapsed = timer.elapsed();
    printf("%.2lf usecond\n", elapsed / 10.0);
    fflush(stdout);
}

void test_console_write()
{
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dis_len(0, 1000);
    std::uniform_int_distribution<int> dis_char{'a', 'z'};
    std::uniform_real_distribution<double> dis_flt(-10000, 1000000);

    STRWNUM stw;

    Timer timer;
    size_t cnt = 0;
    for (int i = 0; i < 10000; ++i)
    {
        std::string str(dis_len(gen), '\0');
        for (auto &j : str)
        {
            j = static_cast<char>(dis_char(gen));
        }
        double d = dis_flt(gen);
        stw = {str, d};
        cnt = cnt + 8 + str.size();
        RECLOG(INFO) << stw;
    }
    double elapsed = timer.elapsed() / 1000;
    printf("Encoded %7.4f kbytes\n", cnt / 1000.0);
    printf("%.2lf seconds, %.2lf MB/s\n", elapsed, (cnt / (1024. * 1024.)) / elapsed);
}

void test_file_write()
{
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dis_len(0, 1000);
    std::uniform_int_distribution<int> dis_char{'a', 'z'};
    std::uniform_real_distribution<double> dis_flt(-10000, 1000000);

    STRWNUM stw;

    Timer timer;
    size_t cnt = 0;
    for (int i = 0; i < 10000; ++i)
    {
        std::string str(dis_len(gen), '\0');
        for (auto &j : str)
        {
            j = static_cast<char>(dis_char(gen));
        }
        double d = dis_flt(gen);
        stw = {str, d};
        cnt = cnt + 8 + str.size();
        RECLOG(INFO) << REFL(stw);
    }
    double elapsed = timer.elapsed() / 1000;
    printf("Encoded %7.4f kbytes\n", cnt / 1000.0);
    printf("%.2lf seconds, %.2lf MB/s\n", elapsed, (cnt / (1024. * 1024.)) / elapsed);
}

TEST(BAGREC, console_write)
{
    RECLOG::INIT_REC();
    test_console_write();
}

TEST(BAGREC, file_write)
{
    RECLOG::INIT_REC("st.cbor");
    test_file_write();
}

TEST(BAGREC, console_write_md)
{
    RECLOG::INIT_REC();
    std::thread th1([]()
                    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        test_console_write(); });
    std::thread th2([]()
                    { test_console_write(); });
    std::thread th3([]()
                    {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        test_console_write(); });
    th1.join();
    th2.join();
    th3.join();
}

TEST(BAGREC, file_write_md)
{
    RECLOG::INIT_REC("st.cbor");
    std::thread th1([]()
                    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        test_file_write(); });
    std::thread th2([]()
                    { test_file_write(); });
    std::thread th3([]()
                    {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        test_file_write(); });
    th1.join();
    th2.join();
    th3.join();
}

TEST(BAGREC, file_read)
{
    ro_disk_file roo("st.cbor");
    if (roo.is_open())
    {
        hd_debug hdb;
        cborio::decoder de(roo, hdb);
        de.run();
        roo.close();
    }
}