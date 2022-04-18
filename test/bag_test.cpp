#include "gtest/gtest.h"
#include "cbor_file.h"
#include "my_class.h"

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
DEFINE_STRUCT(Testwithfunc,
              (int)a,
              (Point)p);

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

TEST(BAGREC, stream_io)
{
    TEST_CBOR tcb = {1, 8.9};
    // encoder
    cbostream cbs;
    uint64_t ces = 887;
    cbs << REFL(tcb) << "cessjo" << 1 << 5.599 << -1 << REFL(ces) << tcb;
    // decoder
    hd_debug hd;
    ro_file ro(cbs.data(), cbs.size());
    cborio::decoder de(ro, hd);
    de.run();
}