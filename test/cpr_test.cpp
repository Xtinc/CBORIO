#include "gtest/gtest.h"
#include "compress.h"
#include "test_tools.h"
#include <fstream>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Compress_TestCase, compress_str)
{
    std::stringstream is("abcdefgabcdefgcdefg"); // 61626364656667
    std::stringstream fis;
    std::stringstream os;
    cbor::compress(is, os);
    for (auto &i : os.str())
    {
        std::cout << hex(static_cast<unsigned char>(i));
    }
    std::cout << std::endl;
    cbor::decompress(os, fis);
    std::cout << fis.str() << std::endl;
}

TEST(Compress_TestCase, compress_file)
{
    Timer timer;
    std::ifstream ifs("../enwik8", std::ios_base::binary);
    std::ofstream ofs("st.lzw", std::ios_base::binary);
    cbor::compress(ifs, ofs);
    std::cout << timer.elapsed() / 1000.0 << std::endl;
}