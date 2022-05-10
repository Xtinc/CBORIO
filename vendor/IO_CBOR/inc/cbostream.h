#include "new_cbor.h"
#include <vector>

namespace details
{
    // Major type
    constexpr uint64_t ucPInt = 0u;
    constexpr uint64_t ucNInt = 1u << 5;
    constexpr uint64_t ucbStr = 2u << 5;
    constexpr uint64_t uctStr = 3u << 5;
    constexpr uint64_t ucArray = 4u << 5;
    constexpr uint64_t ucMap = 5u << 5;
    constexpr uint64_t ucSemantic = 6u << 5;
    constexpr uint64_t ucfloat = 7u << 5;
    constexpr uint64_t ucsimple = 7u << 5;
    constexpr uint64_t ucmask = 0xe0u;

    // details type

    constexpr uint64_t ucLength1 = 24u;
    constexpr uint64_t ucLength2 = 25u;
    constexpr uint64_t ucLength4 = 26u;
    constexpr uint64_t ucLength8 = 27u;

    constexpr uint64_t ucFalse = 20u;
    constexpr uint64_t ucTrue = 21u;
    constexpr uint64_t ucNull = 22u;
    constexpr uint64_t ucUndefined = 23u;
    constexpr uint64_t ucSingleFloat = 26u;
    constexpr uint64_t ucDoubleFloat = 27u;

    constexpr uint64_t ucDataTime = 0u;
    constexpr uint64_t ucEpochDataTime = 1u;
    constexpr uint64_t ucPositiveBignum = 2u;
    constexpr uint64_t ucNegativeBignum = 3u;
    constexpr uint64_t ucDecimalFraction = 4u;
    constexpr uint64_t ucBigfloat = 5u;
    constexpr uint64_t ucConvertBase64Url = 21u;
    constexpr uint64_t ucConvertBase64 = 22u;
    constexpr uint64_t ucConvertBase16 = 23u;
    constexpr uint64_t ucCborEncodedData = 24u;
    constexpr uint64_t ucUri = 32u;
    constexpr uint64_t ucBase64Url = 33u;
    constexpr uint64_t ucBase64 = 34u;
    constexpr uint64_t ucRegex = 35u;
    constexpr uint64_t ucMimeMessage = 36u;
    constexpr uint64_t ucSelfDescribeCbor = 55799u;
    constexpr uint64_t ucMask = 0x1fu;
}

class CborStream
{
public:
    template <typename T>
    CborStream &operator<<(const T &t)
    {
        EncodeData(t);
        return *this;
    }
    std::string &GetStr()
    {
        return m_vBuf;
    }

private:
    std::string m_vBuf;

    template <typename Type>
    typename std::enable_if<std::is_unsigned<Type>::value, std::size_t>::type
    GetLength(Type val)
    {
        if (val < 24)
            return 0;
        for (std::size_t i = 1; i <= ((sizeof(val)) >> 1); i <<= 1)
        {
            if (!(val >> (i << 3)))
                return i;
        }
        return sizeof(val);
    }

    size_t EncodeDirectly(uint64_t tag, uint64_t additional)
    {
        m_vBuf.push_back(static_cast<char>(tag + additional));
        return 1;
    }

    template <typename Type>
    typename std::enable_if<std::is_unsigned<Type>::value, std::size_t>::type
    EncodeTagAndValue(uint64_t tag, const Type t)
    {
        auto len = GetLength(t);
        m_vBuf.reserve(m_vBuf.size() + len + 1);

        switch (len)
        {
        case 8:
            EncodeDirectly(tag, details::ucLength8);
            break;
        case 4:
            EncodeDirectly(tag, details::ucLength4);
            break;
        case 2:
            EncodeDirectly(tag, details::ucLength2);
            break;
        case 1:
            EncodeDirectly(tag, details::ucLength1);
            break;
        case 0:
            return EncodeDirectly(tag, t);
        default:
            // throw Exception("too long");
            break;
        }

        switch (len)
        {
        case 8:
            m_vBuf.push_back((t >> 56) & 0xffU);
            m_vBuf.push_back((t >> 48) & 0xffU);
            m_vBuf.push_back((t >> 40) & 0xffU);
            m_vBuf.push_back((t >> 32) & 0xffU);
        case 4:
            m_vBuf.push_back((t >> 24) & 0xffU);
            m_vBuf.push_back((t >> 16) & 0xffU);
        case 2:
            m_vBuf.push_back((t >> 8) & 0xffU);
        case 1:
            m_vBuf.push_back(t & 0xffU);
        }

        return 1 + len;
    }

    // above are tools.

    size_t EncodeData(const bool &t)
    {
        return EncodeDirectly(details::ucfloat, t ? details::ucTrue : details::ucFalse);
    }

    size_t EncodeData(const char *t)
    {
        auto bytes = strlen(t);
        auto len = EncodeTagAndValue(details::ucbStr, bytes);
        m_vBuf.insert(std::end(m_vBuf), t, t + bytes);
        return len + bytes;
    }

    template <typename Type>
    typename std::enable_if<std::is_unsigned<Type>::value, std::size_t>::type
    EncodeData(const Type &t)
    {
        return EncodeTagAndValue(details::ucPInt, t);
    }

    template <typename Type>
    typename std::enable_if<std::is_signed<Type>::value, std::size_t>::type
    EncodeData(const Type &t)
    {
        return t >= 0 ? EncodeTagAndValue(details::ucPInt, (uint64_t)t) : EncodeTagAndValue(details::ucNInt, (uint64_t)(-t - 1));
    }

    template <typename Type>
    typename std::enable_if<std::is_same<typename Type::value_type, char>::value, std::size_t>::type
    EncodeData(const Type &t)
    {
        auto len = EncodeTagAndValue(details::ucbStr, t.size());
        m_vBuf.insert(std::end(m_vBuf), std::begin(t), std::end(t));
        return len + t.size();
    }

    template <typename Type>
    typename std::enable_if<std::is_same<typename Type::value_type, unsigned char>::value, std::size_t>::type
    EncodeData(const Type &t)
    {
        auto len = encodeTagAndValue(details::uctStr, t.size());
        m_vBuf.insert(std::end(m_vBuf), std::begin(t), std::end(t));
        return len + t.size();
    }

    template <typename Type>
    typename std::enable_if<!std::is_void<typename Type::iterator::value_type>::value, std::size_t>::type
    EncodeData(const Type &t)
    {
        auto len = encodeTagAndValue(details::uctStr, t.size());
        m_vBuf.insert(std::end(m_vBuf), std::begin(t), std::end(t));
        return len + t.size();
    }
};