#include "encoder.h"
namespace cborio
{

    void encoder::write_bool_value(bool value)
    {
        return value ? m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x15)) : m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x14));
    }

    void encoder::write_float_value(float value)
    {
        char *tmp = reinterpret_cast<char *>(&value);
        m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x1A));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 3)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 2)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 1)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp)));
        return;
    }

    void encoder::write_float_value(double value)
    {
        char *tmp = reinterpret_cast<char *>(&value);
        m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x1B));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 7)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 6)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 5)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 4)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 3)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 2)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 1)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp)));
        return;
    }

    void encoder::write_type_value(int major_type, uint64_t value)
    {
        major_type <<= 5;
        if (value <= 0x17)
        {
            m_out.put_byte(static_cast<unsigned char>(major_type | value));
        }
        else if (value <= 0xFF)
        {
            m_out.put_byte(static_cast<unsigned char>(major_type | 0x18));
            m_out.put_byte(static_cast<unsigned char>(value));
        }
        else if (value <= 0xFFFF)
        {
            m_out.put_byte(static_cast<unsigned char>(major_type | 0x19));
            m_out.put_byte(static_cast<unsigned char>(value >> 8));
            m_out.put_byte(static_cast<unsigned char>(value));
        }
        else if (value <= 0xFFFFFFFF)
        {
            m_out.put_byte(static_cast<unsigned char>(major_type | 0x1A));
            m_out.put_byte(static_cast<unsigned char>(value >> 24));
            m_out.put_byte(static_cast<unsigned char>(value >> 16));
            m_out.put_byte(static_cast<unsigned char>(value >> 8));
            m_out.put_byte(static_cast<unsigned char>(value));
        }
        else
        {
            m_out.put_byte(static_cast<unsigned char>(major_type | 0x1B));
            m_out.put_byte(static_cast<unsigned char>(value >> 56));
            m_out.put_byte(static_cast<unsigned char>(value >> 48));
            m_out.put_byte(static_cast<unsigned char>(value >> 40));
            m_out.put_byte(static_cast<unsigned char>(value >> 32));
            m_out.put_byte(static_cast<unsigned char>(value >> 24));
            m_out.put_byte(static_cast<unsigned char>(value >> 16));
            m_out.put_byte(static_cast<unsigned char>(value >> 8));
            m_out.put_byte(static_cast<unsigned char>(value));
        }
        return;
    }

    void encoder::write_array_head(size_t size)
    {
        write_type_value(4, static_cast<unsigned int>(size));
    }

    void encoder::write_null()
    {
        m_out.put_byte(static_cast<unsigned char>(0xf6));
    }

    void encoder::write_map(size_t size)
    {
        write_type_value(5, static_cast<unsigned int>(size));
    }

    void encoder::write_tag(const unsigned int tag)
    {
        write_type_value(6, tag);
    }

    void encoder::write_special(int special)
    {
        write_type_value(7, static_cast<unsigned int>(special));
    }

    void encoder::write_undefined()
    {
        m_out.put_byte(static_cast<unsigned char>(0xf7));
    }
}