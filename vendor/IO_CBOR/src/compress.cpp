#include "encoder.h"
#include <array>
#include <algorithm>
#include <climits>

using CodeType = std::uint32_t;

constexpr CodeType dms{1024 * 512};
constexpr CodeType MetaCode_EOF = 0x00;
constexpr unsigned int ReadBufSize = 512;

class EncoderDictionary
{
    struct Node
    {
        explicit Node(char c) : first(dms), c(c), left(dms), right(dms)
        {
        }

        char c;         ///< Byte.
        CodeType first; ///< Code of first child string.
        CodeType left;  ///< Code of child node with byte < `c`.
        CodeType right; ///< Code of child node with byte > `c`.
    };

public:
    EncoderDictionary()
    {
        const int minc = std::numeric_limits<char>::min();
        const int maxc = std::numeric_limits<char>::max();
        CodeType k{0};

        for (auto c = minc; c <= maxc; ++c)
            initials[static_cast<unsigned char>(c)] = k++;

        vn.reserve(dms);
        reset();
    }

    void reset()
    {
        vn.clear();

        const int minc = std::numeric_limits<char>::min();
        const int maxc = std::numeric_limits<char>::max();

        for (auto c = minc; c <= maxc; ++c)
        {
            vn.push_back(Node(static_cast<char>(c)));
        }

        // add dummy nodes for the metacodes
        vn.push_back(Node('\x00')); //  MetaCode_EOF
    }

    CodeType search_and_insert(CodeType i, char c)
    {
        if (i == dms)
        {
            return search_initials(c);
        }

        const CodeType vn_size = static_cast<uint32_t>(vn.size());
        CodeType ci{vn[i].first}; // Current Index

        if (ci != dms)
        {
            while (true)
            {
                if (c < vn[ci].c)
                {
                    if (vn[ci].left == dms)
                    {
                        vn[ci].left = vn_size;
                        break;
                    }
                    else
                    {
                        ci = vn[ci].left;
                    }
                }
                else if (c > vn[ci].c)
                {
                    if (vn[ci].right == dms)
                    {
                        vn[ci].right = vn_size;
                        break;
                    }
                    else
                    {
                        ci = vn[ci].right;
                    }
                }
                else
                {
                    return ci;
                }
            }
        }
        else
        {
            vn[i].first = vn_size;
        }

        vn.push_back(Node(c));
        return dms;
    }

    CodeType search_initials(char c) const
    {
        return initials[static_cast<unsigned char>(c)];
    }

    std::vector<Node>::size_type size() const
    {
        return vn.size();
    }

private:
    std::vector<Node> vn;
    std::array<CodeType, 1u << CHAR_BIT> initials;
};

struct ByteCache
{

    ByteCache() : used(0), data(0x00)
    {
    }

    std::size_t used;   ///< Bits currently in use.
    unsigned char data; ///< The bits of the cached byte.
};

class CodeWriter
{
public:
    explicit CodeWriter(std::ostream &os) : os(os), bits(CHAR_BIT + 1)
    {
    }

    ~CodeWriter()
    {
        write(static_cast<CodeType>(MetaCode_EOF));

        // write the incomplete leftover byte as-is
        if (lo.used != 0)
        {
            os.put(static_cast<char>(lo.data));
        }
    }

    std::size_t get_bits() const
    {
        return bits;
    }

    void reset_bits()
    {
        bits = CHAR_BIT + 1;
    }

    void increase_bits()
    {
        ++bits;
    }

    bool write(CodeType k)
    {
        std::size_t remaining_bits{bits};

        if (lo.used != 0)
        {
            lo.data |= k << lo.used;
            os.put(static_cast<char>(lo.data));
            k >>= CHAR_BIT - lo.used;
            remaining_bits -= CHAR_BIT - lo.used;
            lo.used = 0;
            lo.data = 0x00;
        }

        while (remaining_bits != 0)
        {
            if (remaining_bits >= CHAR_BIT)
            {
                os.put(static_cast<char>(k));
                k >>= CHAR_BIT;
                remaining_bits -= CHAR_BIT;
            }
            else
            {
                lo.used = remaining_bits;
                lo.data = static_cast<unsigned char>(k);
                break;
            }
        }

        return os.good();
    }

private:
    std::ostream &os; ///< Output Stream.
    std::size_t bits; ///< Binary width of codes.
    ByteCache lo;     ///< LeftOvers.
};

class CodeReader
{
public:
    explicit CodeReader(std::istream &is) : is(is), bits(CHAR_BIT + 1), feofmc(false)
    {
    }

    std::size_t get_bits() const
    {
        return bits;
    }
    void reset_bits()
    {
        bits = CHAR_BIT + 1;
    }

    void increase_bits()
    {
        ++bits;
    }

    bool read(CodeType &k)
    {
        // ready-made bit masks
        static const std::array<unsigned long int, 9> masks{
            {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF}};

        std::size_t remaining_bits{bits};
        std::size_t offset{lo.used};
        unsigned char temp;

        k = lo.data;
        remaining_bits -= lo.used;
        lo.used = 0;
        lo.data = 0x00;

        while (remaining_bits != 0 && is.get(reinterpret_cast<char &>(temp)))
        {
            if (remaining_bits >= CHAR_BIT)
            {
                k |= static_cast<CodeType>(temp) << offset;
                offset += CHAR_BIT;
                remaining_bits -= CHAR_BIT;
            }
            else
            {
                k |= static_cast<CodeType>(temp & masks[remaining_bits]) << offset;
                lo.used = CHAR_BIT - remaining_bits;
                lo.data = temp >> remaining_bits;
                break;
            }
        }

        if (k == static_cast<CodeType>(MetaCode_EOF))
        {
            feofmc = true;
            return false;
        }

        return is.good();
    }

    bool corrupted() const
    {
        return !feofmc;
    }

private:
    std::istream &is; ///< Input Stream.
    std::size_t bits; ///< Binary width of codes.
    bool feofmc;      ///< Found End-Of-File MetaCode.
    ByteCache lo;     ///< LeftOvers.
};

std::size_t RequiredBits(size_t n)
{
    std::size_t r{1};

    while ((n >>= 1) != 0)
    {
        ++r;
    }

    return r;
}

void cborio::compress(std::istream &is, std::ostream &os)
{
    EncoderDictionary ed;
    CodeWriter cw(os);
    CodeType i{dms}; // Index
    char c;
    bool rbwf{false}; // Reset Bit Width Flag
    char cbuf[ReadBufSize] = {0};

    do
    {
        is.read(cbuf, ReadBufSize);
        for (auto j = 0; j < is.gcount(); ++j)
        {
            c = cbuf[j];
            // dictionary's maximum size was reached
            if (ed.size() == dms)
            {
                ed.reset();
                rbwf = true;
            }

            const CodeType temp{i};

            if ((i = ed.search_and_insert(temp, c)) == dms)
            {
                cw.write(temp);
                i = ed.search_initials(c);

                if (RequiredBits(ed.size() - 1) > cw.get_bits())
                {
                    cw.increase_bits();
                }
            }

            if (rbwf)
            {
                cw.reset_bits();
                rbwf = false;
            }
        }
    } while (is.good());
    if (i != dms)
    {
        cw.write(i);
    }
}

void cborio::decompress(std::istream &is, std::ostream &os)
{
    std::vector<std::pair<CodeType, char>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&dictionary]
    {
        dictionary.clear();
        dictionary.reserve(dms);

        const int minc = std::numeric_limits<char>::min();
        const int maxc = std::numeric_limits<char>::max();

        for (auto c = minc; c <= maxc; ++c)
        {
            dictionary.push_back({dms, static_cast<char>(c)});
        }

        // add dummy elements for the metacodes
        dictionary.push_back({0, '\x00'}); //  MetaCode_EOF
    };

    const auto rebuild_string = [&dictionary](CodeType k) -> const std::vector<char> *
    {
        static std::vector<char> s; // String

        s.clear();

        // the length of a string cannot exceed the dictionary's number of entries
        s.reserve(dms);

        while (k != dms)
        {
            s.push_back(dictionary[k].second);
            k = dictionary[k].first;
        }

        std::reverse(s.begin(), s.end());
        return &s;
    };

    reset_dictionary();

    CodeReader cr(is);
    CodeType i{dms}; // Index
    CodeType k;      // Key

    while (true)
    {
        // dictionary's maximum size was reached
        if (dictionary.size() == dms)
        {
            reset_dictionary();
            cr.reset_bits();
        }

        if (RequiredBits(dictionary.size()) > cr.get_bits())
        {
            cr.increase_bits();
        }

        if (!cr.read(k))
        {
            break;
        }

        if (k > dictionary.size())
        {
            throw std::runtime_error("invalid compressed code");
        }

        const std::vector<char> *s; // String

        if (k == dictionary.size())
        {
            dictionary.push_back({i, rebuild_string(i)->front()});
            s = rebuild_string(k);
        }
        else
        {
            s = rebuild_string(k);

            if (i != dms)
                dictionary.push_back({i, s->front()});
        }

        os.write(&s->front(), s->size());
        i = k;
    }

    if (cr.corrupted())
    {
        throw std::runtime_error("corrupted compressed file");
    }
}