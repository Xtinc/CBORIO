#include "compress.h"
#include <cassert>
#include <algorithm>

using namespace cborio;

#define LOGV(level, s, ...)                    \
    do                                         \
    {                                          \
        if (level <= 0)                        \
            fprintf(stderr, s, ##__VA_ARGS__); \
    } while (0);

constexpr int64_t kMaxChunkSize = 1 << 18; // 256k

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
        return 31 - __builtin_clz(v);
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

int BitReader::readBits(int n)
{
    int r = (m_bits >> 1) >> (31 - n);
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
    m_bits = (m_bits << 1) | v;
    ++m_pos;
    if (m_pos >= 8)
    {
        flush();
    }
}

void BitWriter::writeBits(int v, int n)
{
    m_bits = (m_bits << n) | v;
    m_pos += n;
    if (m_pos >= 8)
    {
        flush();
    }
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

void HuffmanEncoder::BuildTable()
{
    Node *q[256];
    int num_symbols = 0;
    for (int i = 0; i < max_symbols; ++i)
    {
        if (m_nodes[i].freq != 0)
        {
            m_nodes[num_symbols] = m_nodes[i];
            q[num_symbols] = &m_nodes[num_symbols];
            ++num_symbols;
        }
    }
    auto binarycomp = [](const Node *l, const Node *r)
    { return l->freq > r->freq; };
    std::make_heap(&q[0], &q[num_symbols], binarycomp);
    // build huffman tree
    for (auto i = num_symbols; i > 1; --i)
    {
        Node *n1 = q[0];
        std::pop_heap(&q[0], &q[i], binarycomp);
        Node *n2 = q[0];
        std::pop_heap(&q[0], &q[i - 1], binarycomp);

        Node *parent = &m_nodes[num_symbols + i];
        parent->freq = n1->freq + n2->freq;
        parent->symbol = -1;
        parent->l = n2;
        parent->r = n1;
        q[i - 2] = parent;
        std::push_heap(&q[0], &q[i - 1], binarycomp);
    }
    walk(q[0], num_symbols == 1 ? 1 : 0);
    // sort leaf nodes
    std::sort(&m_nodes[0], &m_nodes[num_symbols],
              [](const Node &l, const Node &r)
              { return l.freq < r.freq; });
    WriteTable(num_symbols);
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

void HuffmanEncoder::WriteTable(int num_symbols)
{
    const int kSymBits = log2(max_symbols);
    m_writer.writeBits(num_symbols - 1, kSymBits);

    for (auto i = 0; i < num_symbols; ++i)
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
        m_length[symbol] = level;
        m_code[symbol] = code;

        LOGV(2, "code:%s hex:%x level:%d symbol:%d\n",
             toBinary(code, level).c_str(), code, level, symbol);
    }
}
