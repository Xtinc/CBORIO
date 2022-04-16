#include "gtest/gtest.h"
#include "cbor_file.h"
#include "my_class.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
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
    cbs << REFL(tcb) << "cessjo" << 1 << 5.599 << -1 << REFL(ces);
    // decoder
    hd_debug hd;
    ro_file ro(cbs.data(), cbs.size());
    cborio::decoder de(ro, hd);
    de.run();
}