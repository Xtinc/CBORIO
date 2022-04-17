#ifndef CBOR_COMPRESS_H
#define CBOR_COMPRESS_H

#include <string>

// nasm from https://github.com/llvm-mirror/libcxx/blob/9dcbb46826fd4d29b1485f25e8986d36019a6dca/include/support/win32/support.h#L106-L182
// Returns the number of trailing 0-bits in x, starting at the least significant
// bit position. If x is 0, the result is undefined.
inline int llvm_ctzll(unsigned long long mask)
{
    unsigned long where;
// Search from LSB to MSB for first set bit.
// Returns zero if no set bit is found.
#if defined(_LIBCPP_HAS_BITSCAN64)
    (defined(_M_AMD64) || defined(__x86_64__)) if (_BitScanForward64(&where, mask)) return static_cast<int>(where);
#else
    // Win32 doesn't have _BitScanForward64 so emulate it with two 32 bit calls.
    // Scan the Low Word.
    if (_BitScanForward(&where, static_cast<unsigned long>(mask)))
        return static_cast<int>(where);
    // Scan the High Word.
    if (_BitScanForward(&where, static_cast<unsigned long>(mask >> 32)))
        return static_cast<int>(where + 32); // Create a bit offset from the LSB.
#endif
    return 64;
}

inline int llvm_ctzl(unsigned long mask)
{
    unsigned long where;
    // Search from LSB to MSB for first set bit.
    // Returns zero if no set bit is found.
    if (_BitScanForward(&where, mask))
        return static_cast<int>(where);
    return 32;
}

inline int llvm_ctz(unsigned int mask)
{
    // Win32 and Win64 expectations.
    static_assert(sizeof(mask) == 4, "");
    static_assert(sizeof(unsigned long) == 4, "");
    return llvm_ctzl(static_cast<unsigned long>(mask));
}

// Returns the number of leading 0-bits in x, starting at the most significant
// bit position. If x is 0, the result is undefined.
inline int llvm_clzll(unsigned long long mask)
{
    unsigned long where;
// BitScanReverse scans from MSB to LSB for first set bit.
// Returns 0 if no set bit is found.
#if defined(_LIBCPP_HAS_BITSCAN64)
    if (_BitScanReverse64(&where, mask))
        return static_cast<int>(63 - where);
#else
    // Scan the high 32 bits.
    if (_BitScanReverse(&where, static_cast<unsigned long>(mask >> 32)))
        return static_cast<int>(63 -
                                (where + 32)); // Create a bit offset from the MSB.
    // Scan the low 32 bits.
    if (_BitScanReverse(&where, static_cast<unsigned long>(mask)))
        return static_cast<int>(63 - where);
#endif
    return 64; // Undefined Behavior.
}

inline int llvm_clzl(unsigned long mask)
{
    unsigned long where;
    // Search from LSB to MSB for first set bit.
    // Returns zero if no set bit is found.
    if (_BitScanReverse(&where, mask))
        return static_cast<int>(31 - where);
    return 32; // Undefined Behavior.
}

inline int llvm_clz(unsigned int x)
{
    return llvm_clzl(x);
}

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

        uint32_t bits() const { return m_bits; }
        int position() const { return m_pos; }
        uint8_t *end() const { return m_end; }
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

    private:
        void flush();
    };

    class HuffmanEncoder
    {
    private:
        int max_symbols;
        // todo :symol,256,node=2*256
        struct Node
        {
            int freq;
            int symbol;
            Node *l;
            Node *r;
        } m_nodes[512];
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
        BitWriter &writer()
        {
            return m_writer;
        }
        void scan(int symbol)
        {
            ++m_nodes[symbol].freq;
        }
        void encode(int symbol)
        {
            m_writer.writeBits(m_code[symbol], m_len[symbol]);
        }
        int64_t finish()
        {
            return m_writer.finish();
        }
        void buildTable();

    private:
        void writeTable(int num_symbols);
        void buildCodes(int num_symbols);
        void limitLength(int num_symbols);
        void walk(Node *n, int level);
    };

    class HuffmanDecoder
    {
        BitReader m_br;
        int m_symbits;
        int num_symbols;
        int min_codelen;
        int max_codelen;
        int codelen_cnt[17] = {0};

        uint8_t symbol_t[256];
        uint8_t bits_to_sym[0x800];
        uint8_t bits_to_len[0x800];

    public:
        HuffmanDecoder(uint8_t *buffer, uint8_t *end, int sym_bits = 8)
            : m_br(buffer, end),
              m_symbits(sym_bits),
              num_symbols(0),
              min_codelen(255),
              max_codelen(0)
        {
        }

        BitReader &br() { return m_br; }
        void readTable();
        void decode(uint8_t *output, uint8_t *output_end);
        uint8_t decodeOne();

    private:
        void assignCodes();
    };

    int64_t HuffmanCompress(const uint8_t *buf, int64_t len, uint8_t *out);
    void HuffmanDecompress(uint8_t *buf, int64_t len, uint8_t *out, int64_t out_len);
}
#endif