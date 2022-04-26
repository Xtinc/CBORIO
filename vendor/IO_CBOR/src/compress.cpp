#include "compress.h"
#include <cassert>
#include <memory.h>
#include <algorithm>

using namespace cborio;

#define LOGV(level, s, ...)                    \
    do                                         \
    {                                          \
        if (level <= 0)                        \
            fprintf(stderr, s, ##__VA_ARGS__); \
    } while (0);

constexpr int64_t kMaxChunkSize = 1 << 18; // 256k
constexpr int kMaxSymbols = 256;
constexpr int kMaxHuffCodeLength = 11;

std::string toBinary(int v, int size)
{
    std::string result;
    for (int j = 0; j < size; ++j)
    {
        result += ((v >> (size - j - 1)) & 1) ? "1" : "0";
    }
    return result;
}

// https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c
int log2(int v)
{
    if (v > 0)
    {
        return 31 - llvm_clz(v);
    }
    else
    {
        return 0;
    }
}

int BitReader::readBit()
{
    int r = m_bits >> 31;
    m_bits <<= 1;
    ++m_pos;
    return r;
}

// https://stackoverflow.com/questions/18799344/shifting-a-32-bit-integer-by-32-bits
// https://fgiesen.wordpress.com/2018/02/19/reading-bits-in-far-too-many-ways-part-1/

int BitReader::readBits(int n)
{
    // m_bits>>1>>(31-n) instead of >>32-n for n=0
    // avoid case >>32 & >>0
    assert(n >= 0 && n < 32);
    int r = m_bits >> 1 >> (31 - n);
    m_bits <<= n;
    m_pos += n;
    return r;
}

void BitReader::refill()
{
    while (m_pos >= 0)
    {
        m_bits |= (m_cur < m_end ? *m_cur : 0) << m_pos;
        m_pos -= 8;
        ++m_cur;
    }
}

void BitReader::byteAlign()
{
    int extra_bits = m_pos & 7;
    if (extra_bits)
    {
        readBits(8 - extra_bits);
    }
}

void BitWriter::writeBit(int v)
{
    assert(v >= 0);
    m_bits = (m_bits << 1) | v;
    ++m_pos;
    if (m_pos >= 8)
    {
        flush();
    }
}

void BitWriter::writeBits(int v, int n)
{
    assert(v >= 0);
    assert(n > 0 && n <= 32);
    m_bits = (m_bits << 1 << (n - 1)) | v;
    m_pos += n;
    if (m_pos >= 8)
    {
        flush();
    }
}

int64_t BitWriter::finish()
{
    flush();
    assert(m_pos >= 0 && m_pos < 8);
    if (m_pos > 0)
    {
        // Final byte is a bit tricky.  Handle it specially.
        *m_cur = (m_bits & ((1 << m_pos) - 1)) << (8 - m_pos);
        ++m_cur;
        m_pos = 0;
    }
    return m_cur - m_start;
}

void BitWriter::flush()
{
    while (m_pos >= 8)
    {
        m_pos -= 8;
        *m_cur = (m_bits >> m_pos) & 0xFF;
        ++m_cur;
    }
}

// http://cbloomrants.blogspot.com/2010/08/08-12-10-lost-huffman-paper.html
// https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md

void HuffmanEncoder::buildTable()
{
    Node *q[256];
    int num_symbols = 0;
    for (int i = 0; i < max_symbols; ++i)
    {
        if (m_nodes[i].freq)
        {
            m_nodes[num_symbols] = m_nodes[i];
            q[num_symbols] = &m_nodes[num_symbols];
            ++num_symbols;
        }
    }

    auto c = [](const Node *l, const Node *r)
    {
        return l->freq > r->freq;
    };
    std::make_heap(&q[0], &q[num_symbols], c);

    // Build Huffman tree
    for (int i = num_symbols; i > 1; --i)
    {
        Node *n1 = q[0];
        std::pop_heap(&q[0], &q[i], c);
        Node *n2 = q[0];
        std::pop_heap(&q[0], &q[i - 1], c);

        Node *parent = &m_nodes[num_symbols + i];
        parent->freq = n1->freq + n2->freq;
        parent->symbol = -1;
        parent->l = n2;
        parent->r = n1;
        q[i - 2] = parent;
        std::push_heap(&q[0], &q[i - 1], c);
    }

    // Label the distances from the root for the leafs
    walk(q[0], num_symbols == 1 ? 1 : 0);
    // Sort leaf nodes into level order.  This is required
    // for both length limiting and writing the table.
    std::sort(&m_nodes[0], &m_nodes[num_symbols], [](const Node &l, const Node &r)
              { return l.freq < r.freq; });

    // limitLength(num_symbols);
    writeTable(num_symbols);
    buildCodes(num_symbols);
}

void HuffmanEncoder::writeTable(int num_symbols)
{
    const int kSymBits = log2(max_symbols);
    m_writer.writeBits(num_symbols - 1, kSymBits);

    for (int i = 0; i < num_symbols; ++i)
    {
        m_writer.writeBits(m_nodes[i].symbol, kSymBits);
        m_writer.writeBits(m_nodes[i].freq - 1, 4);
    }

    // Byte align after the table
    m_writer.finish();
}

void HuffmanEncoder::buildCodes(int num_symbols)
{
    int code = 0;
    int last_level = -1;
    //LOGV(2, "Write num_symbols %d\n", num_symbols);
    for (int i = 0; i < num_symbols; ++i)
    {
        // Build the binary representation.
        int level = m_nodes[i].freq;
        if (last_level != level)
        {
            if (last_level != -1)
            {
                ++code;
                code <<= (level - last_level);
            }
            last_level = level;
        }
        else
        {
            ++code;
        }

        int symbol = m_nodes[i].symbol;
        m_len[symbol] = level;
        m_code[symbol] = code;

        //LOGV(2, "code:%s hex:%x level:%d symbol:%d\n", toBinary(code, level).c_str(), code, level, symbol);
    }
}

// https://en.wikipedia.org/wiki/Package-merge_algorithm
// http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html

void HuffmanEncoder::limitLength(int num_symbols)
{
    // Limit the maximum code length
    int k = 0;
    int maxk = (1 << kMaxHuffCodeLength) - 1;
    for (int i = num_symbols - 1; i >= 0; --i)
    {
        m_nodes[i].freq = std::min(m_nodes[i].freq, kMaxHuffCodeLength);
        k += 1 << (kMaxHuffCodeLength - m_nodes[i].freq);
    }
    //LOGV(3, "k before: %.6lf\n", k / double(maxk));
    for (int i = num_symbols - 1; i >= 0 && k > maxk; --i)
    {
        while (m_nodes[i].freq < kMaxHuffCodeLength)
        {
            ++m_nodes[i].freq;
            k -= 1 << (kMaxHuffCodeLength - m_nodes[i].freq);
        }
    }
    //LOGV(3, "k pass1: %.6lf\n", k / double(maxk));
    for (int i = 0; i < num_symbols; ++i)
    {
        while (k + (1 << (kMaxHuffCodeLength - m_nodes[i].freq)) <= maxk)
        {
            k += 1 << (kMaxHuffCodeLength - m_nodes[i].freq);
            --m_nodes[i].freq;
        }
    }
    //LOGV(3, "k pass2: %x, %x\n", k, maxk);
}

void HuffmanEncoder::walk(Node *n, int level)
{
    if (n->symbol != -1)
    {
        n->freq = level;
        return;
    }
    walk(n->l, level + 1);
    walk(n->r, level + 1);
}

void HuffmanDecoder::readTable()
{
    m_br.refill();
    num_symbols = m_br.readBits(m_symbits) + 1;

    assert(num_symbols <= kMaxSymbols);

    for (int i = 0; i < num_symbols; ++i)
    {
        m_br.refill();
        int symbol = m_br.readBits(m_symbits);
        int codelen = m_br.readBits(4) + 1;
        //LOGV(2, "sym:%d len:%d\n", symbol, codelen);

        ++codelen_cnt[codelen];
        symbol_t[i] = symbol;
        min_codelen = std::min(min_codelen, codelen);
        max_codelen = std::max(max_codelen, codelen);
    }
    //LOGV(1, "num_sym %d codelen(min:%d, max:%d)\n", num_symbols, min_codelen, max_codelen);
    // https://www.jianshu.com/p/4cbbfed4160b
    // Ensure we catch up to be byte aligned.
    m_br.byteAlign();

    assignCodes();
}

void HuffmanDecoder::decode(uint8_t *output, uint8_t *output_end)
{
    uint8_t *src = m_br.cursor();
    uint8_t *src_end = m_br.end();
    int position = 24;
    uint32_t bits = 0;

    for (;;)
    {
        while (position >= 0)
        {
            bits |= (src < src_end ? *src++ : 0) << position;
            position -= 8;
        }
        int n = bits >> (32 - max_codelen);
        int len = bits_to_len[n];
        *output++ = bits_to_sym[n];
        if (output >= output_end)
        {
            break;
        }
        bits <<= len;
        position += len;
    }
}

uint8_t HuffmanDecoder::decodeOne()
{
    m_br.refill();
    int n = m_br.bits() >> (32 - max_codelen);
    int len = bits_to_len[n];
    m_br.readBits(len);
    return bits_to_sym[n];
}

void HuffmanDecoder::assignCodes()
{
    int p = 0;
    uint8_t *cursym = &symbol_t[0];
    for (int i = min_codelen; i <= max_codelen; ++i)
    {
        int n = codelen_cnt[i];
        if (n)
        {
            int shift = max_codelen - i;
            memset(bits_to_len + p, i, n << shift);
            int m = 1 << shift;
            do
            {
                memset(bits_to_sym + p, *cursym++, m);
                p += m;
            } while (--n);
        }
    }
}

int64_t cborio::HuffmanCompress(const uint8_t *iter_src, int64_t len, uint8_t *iter_dst)
{
    uint8_t *out_start = iter_dst;
    int64_t chunk_size = 1 << 18;
    // necessary for int64?
    for (int64_t start = 0; start < len; start += chunk_size)
    {
        int64_t remaining = std::min(chunk_size, len - start);
        uint8_t *marker = iter_dst;
        iter_dst += 3;

        cborio::HuffmanEncoder encoder(iter_dst);
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.scan(iter_src[i]);
        }
        encoder.buildTable();
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.encode(iter_src[i]);
        }
        int64_t chunk_written = encoder.finish();
        marker[0] = chunk_written & 0xff;
        marker[1] = (chunk_written >> 8) & 0xff;
        marker[2] = (chunk_written >> 16) & 0xff;

        iter_src += remaining;
        iter_dst += chunk_written;
    }
    return iter_dst - out_start;
}

void cborio::HuffmanDecompress(uint8_t *iter_src, int64_t len, uint8_t *iter_dst, int64_t out_len)
{
    int64_t chunk_size = kMaxChunkSize;
    uint8_t *buf_end = iter_src + len;
    while (iter_src < buf_end)
    {
        int compressed_size = iter_src[0] | (iter_src[1] << 8) | (iter_src[2] << 16);
        iter_src += 3;

        HuffmanDecoder decoder(iter_src, iter_src + compressed_size);
        decoder.readTable();
        decoder.decode(iter_dst, iter_dst + std::min(chunk_size, out_len));

        iter_src += compressed_size;
        iter_dst += chunk_size;
        out_len -= chunk_size;
    }
}