#ifndef CBOR_FILE_H
#define CBOR_FILE_H

#include "simple_reflect.h"
#include "encoder.h"
#include <fstream>

constexpr int m_buffer_size = 4096;

struct GFunc_out;

class cbostream : public std::ofstream
{
private:
    class cborbuf : public cborio::output
    {
    private:
        std::vector<unsigned char> m_buffer;

    public:
        cborbuf(unsigned int capacity)
        {
            m_buffer.reserve(capacity);
        };

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
    cborbuf m_buf;
    cborio::encoder en;
    unsigned int cnt;

public:
    cbostream() : m_buf(m_buffer_size), en(m_buf), cnt(0){};

    template <typename T,
              typename std::enable_if<refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    cbostream &operator<<(T &&t)
    {
        ++cnt;
        serializeObj(t.second, t.first);
        write_to_disk();
        return *this;
    }

    template <typename T,
              typename std::enable_if<!refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    cbostream &operator<<(T &&t)
    {
        ++cnt;
        serializeObj(t);
        write_to_disk();
        return *this;
    }

    const unsigned char *data() const
    {
        return m_buf.data();
    }

    size_t size() const
    {
        return m_buf.size();
    }

private:
    friend struct GFunc_out;
    template <typename T,
              typename std::enable_if<!refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
    void serializeObj(const T &obj, const char *fieldName = "")
    {
        if (*fieldName)
        {
            en << fieldName << obj;
        }
        else
        {
            en << obj;
        }
    }

    template <typename T,
              typename std::enable_if<refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
    void serializeObj(const T &obj, const char *fieldName = "")
    {
        en << fieldName << "{";
        refl::forEach(obj, GFunc_out(*this));
        en << "}";
    }

    void write_to_disk();
};

struct GFunc_out
{
private:
    cbostream &out;

public:
    GFunc_out(cbostream &_os) : out(_os)
    {
    }
    template <typename Name, typename Valu>
    void operator()(Name &&fdname, Valu &&value)
    {
        out.serializeObj(value, fdname);
    }
};

#endif