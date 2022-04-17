#include "gtest/gtest.h"
#include "compress.h"
#include "utilities.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

std::unique_ptr<uint8_t> readEnwik8(int64_t &len)
{
    FILE *f = fopen("danmu", "r");
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::unique_ptr<uint8_t> buf;
    buf.reset(new uint8_t[len]);
    fread(buf.get(), 1, len, f);
    return buf;
}

TEST(BITWISE_TestCase, bit_reader)
{
    std::vector<unsigned char> con;
    con.emplace_back(0xff);
    con.emplace_back(0xfe);
    con.emplace_back(0xfd);
    con.emplace_back(0xfc);
    // 0xfffefdfc=4,294,901,244
    cborio::BitReader btr(con.data(), con.data() + con.size());
    EXPECT_EQ(4294901244, btr.bits());
    EXPECT_EQ(1, btr.readBit());
    // 0xFFFDFBF8=4,294,835,192
    EXPECT_EQ(4294835192, btr.bits());
    EXPECT_EQ(127, btr.readBits(7));
    // 0x fefdfc00=4,278,057,984
    EXPECT_EQ(4278057984, btr.bits());
    EXPECT_EQ(33422328, btr.readBits(25));
    EXPECT_EQ(0, btr.bits());
}

TEST(BITWISE_TestCase, bit_writer)
{
    std::vector<unsigned char> con(4, 0);
    cborio::BitWriter btw(con.data());
    for (auto i = 0; i < 8; ++i)
    {
        btw.writeBit(11); // 0 or nonzero.
    }
    EXPECT_EQ(con[0], 0xff);
    btw.writeBits(0xfefdfc, 24);
    EXPECT_EQ(con[1], 0xfe);
    EXPECT_EQ(con[2], 0xfd);
    EXPECT_EQ(con[3], 0xfc);
}

TEST(BITWISE_TestCase, bit_BELE)
{
    uint32_t tmp = 0;
    auto ptr = reinterpret_cast<uint8_t *>(&tmp);
    cborio::BitWriter btw(ptr);
    btw.writeBits(0x000000ff, 32);
    // 4278190080
    EXPECT_EQ(tmp, 4278190080);
    cborio::BitReader btr(ptr, ptr + sizeof(tmp));
    EXPECT_EQ(255, btr.bits());
    EXPECT_EQ(127, btr.readBits(31));
}

TEST(BITWISE_TestCase, bit_io)
{
    std::vector<unsigned char> con_in(100, 0x00);
    for (unsigned char i = 0; i < 100; ++i)
    {
        con_in[i] = i;
    }
    std::vector<unsigned char> con_ou(100, 0x00);
    cborio::BitReader orig_reader(con_in.data(), con_in.data() + con_in.size());
    cborio::BitWriter writer(con_ou.data());
    for (size_t i = 0; i < 8 * con_in.size(); ++i)
    {
        orig_reader.refill();
        writer.writeBit(orig_reader.readBit());
    }
    for (auto &i : con_ou)
    {
        std::cout << hex(i);
    }
    std::cout << std::endl;
}

TEST(HUFFMAN_TEST, huffman_encode)
{
    int64_t len;
    std::unique_ptr<uint8_t> buf = readEnwik8(len);
    // len = 10000;
    printf("Read %ld bytes\n", len);
    std::unique_ptr<uint8_t> out;
    out.reset(new uint8_t[len]);

    int64_t encoded_size;
    {
        Timer timer;
        const int kIters = 1;
        for (int i = 0; i < kIters; ++i)
        {
            encoded_size = cborio::HuffmanCompress(buf.get(), len, out.get());
        }
        double elapsed = timer.elapsed() / 1000;
        printf("Encoded %ld into %ld bytes\n", len, encoded_size);
        printf("%.2lf seconds, %.2lf MB/s\n", elapsed, (len * kIters / (1024. * 1024.)) / elapsed);
    }
}
