#ifndef CBOR_ENCODER_H
#define CBOR_ENCODER_H

#include "templates.h"
#include <cstdint>

namespace cborio
{
    class output
    {
    public:
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
        template <typename T>
        encoder &operator<<(T t)
        {
            write_data(t);
            return *this;
        }
        template <typename T, typename std::enable_if<is_charptr<typename std::decay<T>::type>::value>::type * = nullptr>
        void write_data(T t)
        {
            return write_data(t, strlen(t));
        }
        template <typename T, typename std::enable_if<is_signed<T>::value || is_unsigned<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return t < 0 ? write_type_value(1, static_cast<unsigned long long>(-t - 1)) : write_type_value(0, static_cast<unsigned long long>(t));
        }
        template <typename T, typename std::enable_if<is_boolean<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return write_bool_value(t);
        }
        template <typename T, typename std::enable_if<is_char<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return write_data(&t, 1);
        }
        template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void write_data(T t)
        {
            return write_float_value(t);
        }
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
        }

    private:
        template <typename T1, typename T2>
        void internal_tf(T1 t1, T2 t2, unsigned char *)
        {
            write_type_value(2, t2);
            m_out.put_bytes(t1, t2);
        }
        template <typename T1, typename T2>
        void internal_tf(T1 t1, T2 t2, char *)
        {
            write_type_value(3, t2);
            m_out.put_bytes(reinterpret_cast<const unsigned char *>(t1), t2);
        }
        void write_bool_value(bool value);
        void write_float_value(float value);
        void write_float_value(double value);
        void write_type_value(int major_type, unsigned long long value);
        void write_array_head(int size);
        void write_null();
        void write_map(int size);
        void write_tag(const unsigned int tag);
        void write_special(int special);
        void write_undefined();
    };
}

#endif