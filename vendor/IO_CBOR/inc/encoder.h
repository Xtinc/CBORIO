#ifndef CBOR_ENCODER_H
#define CBOR_ENCODER_H

#include "templates.h"
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace cborio
{
    class output
    {
    public:
        output(){};

        virtual void put_byte(unsigned char value) = 0;

        virtual void put_bytes(const unsigned char *data, size_t size) = 0;
    };

    class ustring : public cborio::output
    {
    private:
        std::vector<unsigned char> m_buffer;

    public:
        explicit ustring(unsigned int capacity)
        {
            m_buffer.reserve(capacity);
        };

        using iterator = std::vector<unsigned char>::iterator;
        using const_iterator = std::vector<unsigned char>::const_iterator;

        iterator begin()
        {
            return m_buffer.begin();
        }

        iterator end()
        {
            return m_buffer.end();
        }

        const_iterator cbegin() const
        {
            return m_buffer.cbegin();
        }

        const_iterator cend() const
        {
            return m_buffer.cend();
        }

        friend std::ostream &operator<<(std::ostream &os, const ustring &str)
        {
            os << std::setw(2) << std::setfill('0') << std::hex;
            for (auto iter = str.cbegin(); iter != str.cend(); ++iter)
            {
                os << (int)(*iter);
            }
            os.clear();
            return os;
        }

        const unsigned char *data() const
        {
            return m_buffer.data();
        }

        size_t size() const
        {
            return m_buffer.size();
        }

        void clear() { return m_buffer.clear(); }

        void put_byte(unsigned char value) override
        {
            m_buffer.emplace_back(value);
        }

        void put_bytes(const unsigned char *data, size_t size) override
        {
            for (size_t i = 0; i < size; ++i)
            {
                m_buffer.emplace_back(*(data + i));
            }
        };
    };

    class encoder
    {
    private:
        output &m_out;

    public:
        encoder(output &out) : m_out(out){};
        ~encoder(){};

        template <typename T>
        encoder &operator<<(const T &t)
        {
            write_data(t);
            return *this;
        }

        void write_data(const bool &t)
        {
            return write_bool_value(t);
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            return t < 0 ? write_type_value(1, static_cast<uint64_t>(-t - 1)) : write_type_value(0, static_cast<uint64_t>(t));
        }

        template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            return write_float_value(t);
        }

        template <typename T, typename std::enable_if<is_charptr<typename std::decay<T>::type>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            return write_data(t, strlen(t));
        }

        template <typename T, typename std::enable_if<ISTList<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            write_array_head(t.size());
            for (auto &i : t)
            {
                write_data(i);
            }
            return;
        }

        template <typename T, typename std::enable_if<ISTRing<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            write_data(t.c_str(), t.size());
        }

        template <typename T, typename std::enable_if<ISTLmap<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            write_map(t.size());
            for (auto &i : t)
            {
                write_data(i.first);
                write_data(i.second);
            }
        }

        template <typename T, typename std::enable_if<
                                  !std::is_fundamental<T>::value &&
                                  IsOverloadedOperator<T>::value &&
                                  !is_charptr<typename std::decay<T>::type>::value &&
                                  !ISTList<T>::value &&
                                  !ISTLmap<T>::value &&
                                  !ISTRing<T>::value>::type * = nullptr>
        void write_data(const T &t)
        {
            std::stringstream ss;
            ss << t;
            return write_data(ss.str());
        }

        template <typename T, typename T2,
                  typename std::enable_if<std::is_integral<T2>::value>::type * = nullptr>
        void write_data(const T &t, T2 s)
        {
            using internal_T = typename std::decay<T>::type;
            return internal_tf(t, s, typename CharDispatch<internal_T>::Tag{});
        }

    private:
        template <typename T1, typename T2>
        void internal_tf(const T1 &t1, T2 t2, unsigned char *)
        {
            write_type_value(2, t2);
            m_out.put_bytes(t1, t2);
        }
        template <typename T1, typename T2>
        void internal_tf(const T1 &t1, T2 t2, char *)
        {
            write_type_value(3, t2);
            m_out.put_bytes(reinterpret_cast<const unsigned char *>(t1), t2);
        }
        void write_bool_value(bool value);
        void write_float_value(float value);
        void write_float_value(double value);
        void write_type_value(int major_type, uint64_t value);
        void write_array_head(size_t size);
        void write_null();
        void write_map(size_t size);
        void write_tag(const unsigned int tag);
        void write_special(int special);
        void write_undefined();
    };

    class cborstream : public encoder
    {
    public:
        cborstream() : m_buf(4096), encoder(m_buf) {}
        ~cborstream() {}
        const ustring &u_str()
        {
            return m_buf;
        }

    private:
        ustring m_buf;
    };

    void compress(std::istream &is, std::ostream &os);

    void decompress(std::istream &is, std::ostream &os);
}

#endif