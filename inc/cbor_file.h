#ifndef CBOR_FILE_H
#define CBOR_FILE_H

#include "simple_reflect.h"
#include "encoder.h"

struct GFunc_out;

class cbostream
{
private:
    class cborbuf : public cborio::output
    {
    private:
        std::vector<unsigned char> m_buffer;

    public:
        cborbuf(unsigned int capacity)
            : m_buffer(capacity, 0){};

        const unsigned char *data() const
        {
            return m_buffer.data();
        }

        size_t size() const
        {
            return m_buffer.size();
        }

        void put_byte(unsigned char value) override
        {
            m_buffer.emplace_back(value);
        }

        void put_bytes(const unsigned char *data, int size) override
        {
            for (auto i = 0; i < size; ++i)
            {
                m_buffer.emplace_back(*(data + i));
            }
        };
    };
    cborbuf m_buf;
    cborio::encoder en;

public:
    cbostream() : m_buf(0), en(m_buf){};

    template <typename T,
              typename std::enable_if<refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    cbostream &operator<<(T &&t)
    {
        serializeObj(t.second, t.first);
        return *this;
    }

    template <typename T,
              typename std::enable_if<!refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    cbostream &operator<<(T &&t)
    {
        en << t;
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
        en << fieldName << obj;
    }

    template <typename T,
              typename std::enable_if<refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
    void serializeObj(const T &obj, const char *fieldName = "")
    {
        en << fieldName << (*fieldName ? ": {" : "{");
        refl::forEach(obj, GFunc_out(*this));
    }
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