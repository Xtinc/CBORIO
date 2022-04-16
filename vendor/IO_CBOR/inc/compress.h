#ifndef CBOR_COMPRESS_H
#define CBOR_COMPRESS_H

#include <string>

namespace cborio
{
    class BitReader
    {
    private:
        uint8_t *m_cur;
        uint8_t *m_end;
        uint32_t m_bits;
        int m_pos;

    public:
        BitReader(uint8_t *buf_start, uint8_t *buf_end)
            : m_cur(buf_start),
              m_end(buf_end),
              m_bits(0),
              m_pos(24)
        {
            refill();
        }
        BitReader(uint8_t *buf_start, uint8_t *buf_end, int offset)
            : m_cur(buf_start),
              m_end(buf_end),
              m_bits(0),
              m_pos(24)
        {
            m_pos += offset;
            refill();
        }

        int readBit();
        int readBits(int n);
        void refill();
        void byteAlign();

        uint8_t *current() const { return m_cur; }
        uint8_t *end() const { return m_end; }
        uint32_t bits() const { return m_bits; }
        int position() const { return m_pos; }
        uint8_t *cursor() const
        {
            return m_cur - ((24 - m_pos) / 8);
        }
    };

    class BitWriter
    {
    private:
        uint8_t *start_;
        uint8_t *m_cur;
        uint32_t m_bits;
        int m_pos;

    public:
        BitWriter(uint8_t *buffer)
            : start_(buffer),
              m_cur(buffer),
              m_bits(0),
              m_pos(0)

        {
        }

        void writeBit(int v);
        void writeBits(int v, int n);
        int64_t finish();

    private:
        void flush();
    };

    const int kMaxHuffCodeLength = 11;

    class HuffmanEncoder
    {
    private:
        struct Node
        {
            int freq;
            int symbol;
            Node *l;
            Node *r;
        } m_nodes[512];
        int max_symbols;
        int m_code[256];
        uint8_t m_len[256];
        BitWriter m_writer;

    public:
        HuffmanEncoder(uint8_t *buffer, int max = 256)
            : max_symbols(max),
              m_writer(buffer)
        {
            for (int i = 0; i < max_symbols; ++i)
            {
                m_nodes[i].symbol = i;
                m_nodes[i].freq = 0;
            }
        }
        BitWriter &writer() { return m_writer; }
        void scan(int symbol)
        {
            ++m_nodes[symbol].freq;
        }
        void buildTable();
        void encode(int symbol)
        {
            m_writer.writeBits(m_code[symbol], m_len[symbol]);
        }
        int64_t finish()
        {
            return m_writer.finish();
        }

    private:
        void writeTable(int num_symbols);
        void buildCodes(int num_symbols);
        void limitLength(int num_symbols);
        void walk(Node *n, int level);
    };

    int64_t HuffmanCompress(uint8_t *buf, int64_t len, uint8_t *out);
}
#endif