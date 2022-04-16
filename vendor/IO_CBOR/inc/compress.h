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
        // offset adjusts the stream position before reading any bits.  This allows
        // the BitReader to resume a "backward" stream, which doesn't have to end on
        // a byte boundary.
        BitReader(uint8_t *buffer, uint8_t *end)
            : m_cur(buffer),
              m_end(end),
              m_bits(0),
              m_pos(24)
        {
            refill();
        }
        BitReader(uint8_t *buffer, uint8_t *end, int offset)
            : m_cur(buffer),
              m_end(end),
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

        // Actual location we have read up to in the byte stream.
        uint8_t *cursor() const
        {
            return m_cur - ((24 - m_pos) / 8);
        }
    };

    class BitWriter
    {
    private:
        uint8_t *m_start;
        uint8_t *m_cur;
        uint32_t m_bits;
        int m_pos;

        void flush();

    public:
        BitWriter(uint8_t *buffer)
            : m_start(buffer),
              m_cur(buffer),
              m_bits(0),
              m_pos(0)
        {
        }
        void writeBit(int v);
        void writeBits(int v, int n);
        int64_t finish();
    };

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
        uint8_t m_length[256];
        int m_code[256];
        int max_symbols;
        BitWriter m_writer;

    public:
        HuffmanEncoder(uint8_t *buffer, int max_syl = 256)
            : max_symbols(max_syl),
              m_writer(buffer)
        {
            for (auto i = 0; i < max_symbols; ++i)
            {
                m_nodes[i].symbol = i;
                m_nodes[i].freq = 0;
            }
        }
        BitWriter &writer() { return m_writer; }
        void scan(int symbol) { ++m_nodes[symbol].freq; }
        void encode(int sym) { m_writer.writeBits(m_code[sym], m_length[sym]); }
        int64_t finish() { return m_writer.finish(); }
        void BuildTable();
        void walk(Node *n, int level);
        void WriteTable(int num_symbols);
        void buildCodes(int num_symbols);
    };
}
#endif