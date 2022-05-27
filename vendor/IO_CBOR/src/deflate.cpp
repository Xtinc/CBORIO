#include "deflate.h"
#include <algorithm>
#include <assert.h>

const int MAX_HUFFMAN_CODE_LENGTH = 11;
const int MAX_CHUNK_SIZE = 1 << 18;

BitReader::BitReader(uint8_t *start, uint8_t *end) : current_(start), end_(end)
{
    Refill();
}

int BitReader::ReadBit()
{
    int r = bits_ >> 31;
    bits_ <<= 1;
    return r;
}

int BitReader::ReadBits(int n)
{
    // why not rsh 32 ,see
    // https://fgiesen.wordpress.com/2018/02/19/reading-bits-in-far-too-many-ways-part-1
    int r = (bits_ >> 1) >> (31 - n);
    bits_ <<= n;
    position_ += n;
    return r;
}

void BitReader::Refill()
{
    while (position_ >= 0)
    {
        bits_ |= (current_ < end_ ? *current_ : 0) << position_;
        position_ -= 8;
        ++current_;
    }
}

void BitReader::ByteAlign()
{
    // equal to pos%8;
    int extra_bits = position_ & 7;
    if (extra_bits)
    {
        ReadBits(8 - extra_bits);
    }
}

BitWriter::BitWriter(uint8_t *buffer)
    : start_(buffer), current_(buffer)
{
}

void BitWriter::WriteBit(int v)
{
    bits_ = (bits_ << 1) | v;
    ++position_;
    if (position_ > 8)
    {
        Flush();
    }
}

void BitWriter::WriteBits(int v, int n)
{
    bits_ = (bits_ << n) | v;
    position_ += n;
    if (position_ >= 8)
    {
        Flush();
    }
}

int64_t BitWriter::Finish()
{
    Flush();
    assert(position_ >= 0 && position_ < 8);
    if (position_ > 0)
    {
        *current_ = (bits_ & ((1 << position_) - 1)) << (8 - position_);
        ++current_;
        position_ = 0;
    }
    return current_ - start_;
}

void BitWriter::Flush()
{
    while (position_ > 8)
    {
        position_ -= 8;
        *current_ = (bits_ >> position_) & 0xFF;
        ++current_;
    }
}

HuffmanTree::HuffmanTree(uint8_t *buffer, int max_symbols = 256)
    : writer_(buffer), max_symbols_(max_symbols)
{
    for (int i = 0; i < max_symbols_; ++i)
    {
        nodes_[i].symbol = i;
        nodes_[i].freq = 0;
    }
}

void HuffmanTree::BuildTable()
{
    Node *q[256];
    int num_symbols = 0;
    for (int i = 0; i < max_symbols_; ++i)
    {
        if (nodes_[i].freq)
        {
            nodes_[num_symbols] = nodes_[i];
            q[num_symbols] = &nodes_[num_symbols];
            ++num_symbols;
        }
    }
    auto BiComparor = [](const Node *&l, const Node *&r)
    { return l->freq > r->freq; };
    auto BiComparor2 = [](const Node *&l, const Node *&r)
    { return l->freq < r->freq; };
    std::make_heap(&q[0], &q[num_symbols], BiComparor);
    // build tree
    for (auto i = num_symbols; i > 1; --i)
    {
        Node *n1 = q[0];
        std::pop_heap(&q[0], &q[i], BiComparor);
        Node *n2 = q[0];
        std::pop_heap(&q[0], &q[i - 1], BiComparor);

        Node *parent = &nodes_[num_symbols + i];
        parent->freq = n1->freq + n2->freq;
        parent->symbol = -1;
        parent->l = n2;
        parent->r = n1;
        q[i - 2] = parent;
        std::push_heap(&q[0], &q[i - 1], BiComparor);
    }
    // get depth
    Walk(q[0], num_symbols == 1 ? 1 : 0);
    // sort nodes into level order.
    std::sort(&nodes_[0], &nodes_[num_symbols], BiComparor2);

    LimitLength(num_symbols);
    WriteTable(num_symbols);
    BuildCodes(num_symbols);
}

void HuffmanTree::BuildCodes(int num_symbols)
{
    int code = 0;
    int last_level = -1;
    for (auto i = 0; i < num_symbols; ++i)
    {
        int level = nodes_[i].freq;
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
        int symbol = nodes_[i].symbol;
        length_[symbol] = level;
        code_[symbol] = code;
    }
}

void HuffmanTree::WriteTable(int num_symbols)
{
    const int MAX_SYM_BITS = log2(num_symbols);
    // num_sym - 1 to ensure not overflow.
    writer_.WriteBits(num_symbols - 1, MAX_SYM_BITS);
    for (auto i = 0; i < num_symbols; ++i)
    {
        writer_.WriteBits(nodes_[i].symbol, MAX_SYM_BITS);
        writer_.WriteBits(nodes_[i].freq - 1, 4);
    }
    writer_.Finish();
}

void HuffmanTree::LimitLength(int num_symbols)
{
    // Limit the maximum code length
    int k = 0;
    int maxk = (1 << MAX_HUFFMAN_CODE_LENGTH) - 1;
    for (int i = num_symbols - 1; i >= 0; --i)
    {
        nodes_[i].freq = std::min(nodes_[i].freq, MAX_HUFFMAN_CODE_LENGTH);
        k += 1 << (MAX_HUFFMAN_CODE_LENGTH - nodes_[i].freq);
    }
    for (int i = num_symbols - 1; i >= 0 && k > maxk; --i)
    {
        while (nodes_[i].freq < MAX_HUFFMAN_CODE_LENGTH)
        {
            ++nodes_[i].freq;
            k -= 1 << (MAX_HUFFMAN_CODE_LENGTH - nodes_[i].freq);
        }
    }
    for (int i = 0; i < num_symbols; ++i)
    {
        while (k + (1 << (MAX_HUFFMAN_CODE_LENGTH - nodes_[i].freq)) <= maxk)
        {
            k += 1 << (MAX_HUFFMAN_CODE_LENGTH - nodes_[i].freq);
            --nodes_[i].freq;
        }
    }
}

ParserHuffmanTree::ParserHuffmanTree(uint8_t *buffer, uint8_t *end, int sym_bits = 8)
    : br_(buffer, end), sym_bits_(sym_bits)
{
}

void ParserHuffmanTree::AssignCodes()
{
    int p = 0;
    uint8_t *cursym = &symbol_[0];
    for (auto i = min_codelen_; i <= max_codelen_; ++i)
    {
        int n = codelen_count_[i];
        if (n)
        {
            int shift = max_codelen_ - i;
            memset(bits_to_len_ + p, i, n << shift);
            int m = 1 << shift;
            do
            {
                memset(bits_to_sym_ + p, *cursym++, m);
                p += m;
            } while (--n);
        }
    }
}

void ParserHuffmanTree::ReadTable()
{
    br_.Refill();
    num_symbols_ = br_.ReadBits(sym_bits_) + 1;
    assert(num_symbols_ < kMaxSymbols);
    for (auto i = 0; i < num_symbols_; ++i)
    {
        br_.Refill();
        int symbol = br_.ReadBits(sym_bits_);
        int codelen = br_.ReadBits(4) + 1;
        ++codelen_count_[codelen];
        symbol_[i] = symbol;
        min_codelen_ = std::min(min_codelen_, codelen);
        max_codelen_ = std::max(max_codelen_, codelen);
    }
    br_.ByteAlign();
    AssignCodes();
}

uint8_t ParserHuffmanTree::DecodeOne()
{
    br_.Refill();
    int n = br_.bits() >> (32 - max_codelen_);
    int len = bits_to_len_[n];
    br_.ReadBits(len);
    return bits_to_sym_[n];
}

// http://fastcompression.blogspot.com/2014/02/fse-distributing-symbol-values.html
void ParserHuffmanTree::Decode(uint8_t *output, uint8_t *output_end)
{
    uint8_t *src = br_.cursor();
    uint8_t *src_end = br_.end();
    int position = 24;
    uint32_t bits = 0;

    for (;;)
    {
        while (position > 0)
        {
            bits |= (src < src_end ? *src++ : 0) << position;
            position -= 8;
        }
        int n = bits >> (32 - max_codelen_);
        int len = bits_to_len_[n];
        *output++ = bits_to_sym_[n];
        if (output > output_end)
        {
            break;
        }
        bits <<= len;
        position += len;
    }
}

int64_t Compress_Huffman(const uint8_t *buf, int64_t len, uint8_t *out)
{
    uint8_t *out_start = out;
    int64_t chunk_size = MAX_CHUNK_SIZE;
    for (int64_t start = 0; start < len; start += chunk_size)
    {
        int64_t remaining = std::min(chunk_size, len - start);
        uint8_t *marker = out;
        out += 3;

        HuffmanTree encoder(out);
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.Scan(buf[i]);
        }
        encoder.BuildTable();
        for (int64_t i = 0; i < remaining; ++i)
        {
            encoder.Encoder(buf[i]);
        }
        int64_t chunk_written = encoder.Finish();
        marker[0] = chunk_written & 0xFF;
        marker[1] = (chunk_written >> 8) & 0xFF;
        marker[2] = (chunk_written >> 16) & 0xFF;
        buf += remaining;
        out += chunk_written;
    }
    return out - out_start;
}

void Decompress_Huffman(uint8_t *buf, int64_t len, uint8_t *out, int64_t out_len)
{
    int64_t read = 0;
    int64_t chunk_size = MAX_CHUNK_SIZE;
    uint8_t *buf_end = buf + len;
    while (buf < buf_end)
    {
        int compressed_size = buf[0] | (buf[1] << 8) | (buf[2] << 16);
        buf += 3;

        ParserHuffmanTree decoder(buf, buf + compressed_size);
        decoder.ReadTable();
        decoder.Decode(out, out + std::min(chunk_size, out_len));

        buf += compressed_size;
        out += chunk_size;
        out_len -= chunk_size;
    }
}