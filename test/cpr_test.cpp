#include "gtest/gtest.h"
#include "compress.h"
#include <chrono>

// http://mattmahoney.net/dc/textdata.html
struct Timer
{
    Timer() : time_(std::chrono::high_resolution_clock::now()) {}
    double elapsed() const { return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - time_).count(); }
    std::chrono::high_resolution_clock::time_point time_;
};

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

std::unique_ptr<uint8_t> readEnwik8(int64_t &len)
{
    FILE *f = fopen("enwik8", "r");
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::unique_ptr<uint8_t> buf;
    buf.reset(new uint8_t[len]);
    fread(buf.get(), 1, len, f);
    return buf;
}

TEST(CPR_TEST, huffman_encode)
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
