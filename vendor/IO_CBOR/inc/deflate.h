#ifndef DEFLATE_H
#define DEFLATE_H

#include <string>

class BitReader
{
public:
    BitReader(uint8_t *buffer, uint8_t *end);

    // offset adjusts the stream position before reading any bits.  This allows
    // the BitReader to resume a "backward" stream, which doesn't have to end on
    // a byte boundary.
    BitReader(uint8_t *buffer, uint8_t *end, int offset);

    int ReadBit();

    int ReadBits(int n);

    void Refill();

    void ByteAlign();

    uint8_t *current() const { return current_; }
    uint8_t *end() const { return end_; }
    uint32_t bits() const { return bits_; }
    int position() const { return position_; }

    // Actual location we have read up to in the byte stream.
    uint8_t *cursor() const
    {
        return current_ - ((24 - position_) / 8);
    }

private:
    uint8_t *current_;
    uint8_t *end_;
    uint32_t bits_ = 0;
    int position_ = 24;
};

class BitWriter
{
public:
    BitWriter(uint8_t *buffer);

    void WriteBit(int v);

    void WriteBits(int v, int n);

    int64_t Finish();

private:
    void Flush();

private:
    uint8_t *start_;
    uint8_t *current_;
    uint32_t bits_ = 0;
    int position_ = 0;
};

class HuffmanTree
{
private:
    struct Node
    {
        int freq;
        int symbol;
        Node *l;
        Node *r;
    };

public:
    HuffmanTree(uint8_t *buffer, int max_symbols = 256);
    BitWriter &Writer() { return writer_; }

    void BuildTable();

    void Scan(int symbol)
    {
        ++nodes_[symbol].freq;
    }

    void Encoder(int symbol)
    {
        writer_.WriteBits(code_[symbol], length_[symbol]);
    }

    int64_t Finish()
    {
        return writer_.Finish();
    }

private:
    void WriteTable(int num_symbols);
    void BuildCodes(int num_symbols);
    void LimitLength(int num_symbols);
    void Walk(Node *n, int level)
    {
        if (n->symbol != -1)
        {
            n->freq = level;
            return;
        }
        Walk(n->l, level + 1);
        Walk(n->r, level + 1);
    }

private:
    BitWriter writer_;
    int max_symbols_;
    Node nodes_[512];
    uint8_t length_[256];
    int code_[256];
};

class ParserHuffmanTree
{
public:
    ParserHuffmanTree(uint8_t *buffer, uint8_t *end, int sym_bits = 8);

    BitReader &br() { return br_; }

    void ReadTable();

    void Decode(uint8_t *output, uint8_t *output_end);

    uint8_t DecodeOne();

    static const int kMaxSymbols = 256;

private:
    void AssignCodes();

    BitReader br_;
    int sym_bits_;
    int num_symbols_;
    int min_codelen_ = 255;
    int max_codelen_ = 0;
    int codelen_count_[17] = {0};

    uint8_t symbol_[256];
    uint8_t bits_to_sym_[0x800];
    uint8_t bits_to_len_[0x800];
};

int64_t Compress_Huffman(uint8_t *buf, int64_t len, uint8_t *out);

void Decompress_Huffman(uint8_t *buf, int64_t len, uint8_t *out, int64_t out_len);

#endif