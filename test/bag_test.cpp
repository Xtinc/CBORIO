#include "gtest/gtest.h"
#include "cbor_file.h"
#include "my_class.h"
#include "test_tools.h"
#include "utilities.h"
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

TEST(BAGREC, stream_io)
{
    TEST_CBOR tcb = {1, 8.9};
    // encoder
    recfile cbs(getFILE());
    uint64_t ces = 887;
    cbs << REFL(tcb) << "cessjo" << 1 << 5.599 << -1 << REFL(ces) << tcb;
    // decoder
    hd_debug hd;
    ro_file ro(cbs.data(), cbs.size());
    cborio::decoder de(ro, hd);
    de.run();
}

void test_file_write()
{
    std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<int> dis_len(0, 1000);
    std::uniform_int_distribution<int> dis_char{'a', 'z'};
    std::uniform_real_distribution<double> dis_flt(-10000, 1000000);

    STRWNUM stw;
    recfile cbs(getFILE());

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
        cbs << REFL(stw);
    }
    double elapsed = timer.elapsed() / 1000;
    printf("Encoded %7.4f kbytes\n", cnt / 1000.0);
    printf("%.2lf seconds, %.2lf MB/s\n", elapsed, (cnt / (1024. * 1024.)) / elapsed);
}

TEST(BAGREC, file_write)
{
    test_file_write();
}

TEST(BAGREC, file_write_md)
{
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