#ifndef CBOR_ENCODER_H
#define CBOR_ENCODER_H

#include "templates.h"
#include <cstdint>

namespace cborio
{
    class output
    {
    public:
        // virtual unsigned char *data() = 0;

        // virtual unsigned int size() = 0;
        output(){};

        virtual void put_byte(unsigned char value) = 0;

        virtual void put_bytes(const unsigned char *data, int size) = 0;
    };

    class encoder
    {
    private:
        output &m_out;

    public:
        encoder(output &out) : m_out(out){};
        ~encoder(){};
        template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return t < 0 ? write_type_value(1, static_cast<unsigned long long>(-t - 1)) : write_type_value(0, static_cast<unsigned long long>(t));
        };
        template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return t < 0 ? write_type_value(1, static_cast<unsigned long long>(-t - 1)) : write_type_value(0, static_cast<unsigned long long>(t));
        };
        template <typename T, typename std::enable_if<ISTList<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            write_array_head(t.size());
            for (auto &i : t)
            {
                write_data(i);
            }
            return;
        }
        template <typename T, typename std::enable_if<ISTRing<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            write_data(t.c_str(), t.size());
        }
        template <typename T, typename std::enable_if<ISTLmap<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            write_map(t.size());
            for (auto &i : t)
            {
                write_data(i.first);
                write_data(i.second);
            }
        }
        template <typename T, typename T2,
                  typename std::enable_if<std::is_integral<T2>::value>::type * = nullptr>
        void write_data(T t, T2 s)
        {
            using internal_T = typename std::decay<T>::type;
            return internal_tf(t, s, typename CharDispatch<internal_T>::Tag{});
        };

    private:
        template <typename T1, typename T2>
        void internal_tf(T1 t1, T2 t2, unsigned char *by)
        {
            write_type_value(2, t2);
            m_out.put_bytes(t1, t2);
        }
        template <typename T1, typename T2>
        void internal_tf(T1 t1, T2 t2, char *by)
        {
            write_type_value(3, t2);
            m_out.put_bytes(reinterpret_cast<unsigned char *>(const_cast<char *>(t1)), t2);
        }

        void write_type_value(int major_type, unsigned long long value)
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
        void write_array_head(int size)
        {
            write_type_value(4, static_cast<unsigned int>(size));
        }
        void write_null()
        {
            m_out.put_byte(static_cast<unsigned char>(0xf6));
        }
        void write_map(int size)
        {
            write_type_value(5, static_cast<unsigned int>(size));
        }
        void write_tag(const unsigned int tag)
        {
            write_type_value(6, tag);
        }
        void write_special(int special)
        {
            write_type_value(7, static_cast<unsigned int>(special));
        }
        void write_undefined()
        {
            m_out.put_byte(static_cast<unsigned char>(0xf7));
        }
    };

    template <>
    void encoder::write_data(bool value)
    {
        return value ? m_out.put_byte(static_cast<unsigned char>(0xF5)) : m_out.put_byte(static_cast<unsigned char>(0xF4));
    }
    template <>
    void encoder::write_data(float value)
    {
        char *tmp = reinterpret_cast<char *>(&value);
        m_out.put_byte(static_cast<unsigned char>(0xFA));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 3)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 2)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 1)));
        m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp)));
        return;
    }
    template <>
    void encoder::write_data(double value)
    {
        char *tmp = reinterpret_cast<char *>(&value);
        m_out.put_byte(static_cast<unsigned char>(0xFB));
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
}

#endif