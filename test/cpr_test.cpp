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

int64_t HuffmanCompress(uint8_t *buf, int64_t len, uint8_t *out)
{
    uint8_t *out_start = out;
    int64_t chunk_size = 1 << 18;
    for (int64_t start = 0; start < len; start += chunk_size)
    {
        int64_t remaining = std::min(chunk_size, len - start);
        uint8_t *marker = out;
        out += 3;

        cborio::HuffmanEncoder encoder(out);
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.scan(buf[i]);
        }
        encoder.BuildTable();
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.encode(buf[i]);
        }
        int64_t chunk_written = encoder.finish();
        marker[0] = chunk_written & 0xff;
        marker[1] = (chunk_written >> 8) & 0xff;
        marker[2] = (chunk_written >> 16) & 0xff;

        buf += remaining;
        out += chunk_written;
    }
    return out - out_start;
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

TEST(cpr_test, huffman_encode)
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
            encoded_size = HuffmanCompress(buf.get(), len, out.get());
        }
        double elapsed = timer.elapsed() / 1000;
        printf("Encoded %ld into %ld bytes\n", len, encoded_size);
        printf("%.2lf seconds, %.2lf MB/s\n", elapsed, (len * kIters / (1024. * 1024.)) / elapsed);
    }
}
