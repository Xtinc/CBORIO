#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include <sstream>

#define RECLOG reconsole(RECONFIG::fp, __FILE__, __LINE__)

void INIT_REC(const char *filename = "");

constexpr int m_buffer_size = 4096;

struct GFunc_out;

class reconsole
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
    FILE *mpFile;

private:
    const char *_file;
    unsigned _line;
    std::ostringstream _ss;

public:
    reconsole(FILE *fp, const char *file, unsigned line)
        : m_buf(m_buffer_size), en(m_buf), mpFile(fp), _file(file), _line(line) {}
    ~reconsole();

    template <typename T,
              typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
    reconsole &operator<<(T &&t)
    {
        serializeObj(t.st_t, t.st_name);
        return *this;
    }

    template <typename T,
              typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
    reconsole &operator<<(T &&t)
    {
        serializeObj(t);
        return *this;
    }

private:
    friend struct GFunc_out;
    template <typename T,
              typename std::enable_if<!refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
    void serializeObj(const T &obj, const char *fieldName = "")
    {
        if (mpFile == nullptr)
        {
            if (*fieldName)
            {
                _ss << fieldName << obj;
            }
            else
            {
                _ss << obj;
            }
        }
        else
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
    }

    template <typename T,
              typename std::enable_if<refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
    void serializeObj(const T &obj, const char *fieldName = "")
    {
        if (mpFile == nullptr)
        {
            _ss << fieldName << '{';
            refl::forEach(obj, GFunc_out(*this));
            _ss << '}';
        }
        else
        {
            en << fieldName << '{';
            refl::forEach(obj, GFunc_out(*this));
            en << '}';
        }
    }

    // std::endl and other iomanip:s.
    reconsole &operator<<(std::ostream &(*f)(std::ostream &))
    {
        if (mpFile == nullptr)
        {
            f(_ss);
        }
        return *this;
    }
};
/*
class recfile
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
    FILE *mpFile;

public:
    recfile(FILE *fp) : m_buf(m_buffer_size), en(m_buf), mpFile(fp){};

    template <typename T,
              typename std::enable_if<refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    recfile &operator<<(T &&t)
    {
        serializeObj(t.second, t.first);
        write_to_disk();
        return *this;
    }

    template <typename T,
              typename std::enable_if<!refl::is_pair<typename std::decay<T>::type>::value>::type * = nullptr>
    recfile &operator<<(T &&t)
    {
        serializeObj(t);
        write_to_disk();
        return *this;
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
        en << fieldName << '{';
        refl::forEach(obj, GFunc_out(*this));
        en << '}';
    }

    void set_date_thread();

    void write_to_disk();
};*/

struct GFunc_out
{
private:
    reconsole &out;

public:
    GFunc_out(reconsole &_os) : out(_os)
    {
    }
    template <typename Name, typename Valu>
    void operator()(Name &&fdname, Valu &&value)
    {
        out.serializeObj(value, fdname);
    }
};

#endif