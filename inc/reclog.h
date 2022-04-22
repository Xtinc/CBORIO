#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include "utilities.h"
#include <sstream>

#define RECLOG reconsole(__FILE__, __LINE__)
#define RECDSK recfile(getFILE())

constexpr int m_buffer_size = 4096;

class reconsole
{
public:
    reconsole(const char *file, unsigned line) : _file(file), _line(line) {}
    ~reconsole() noexcept(false)
    {
        char preamble_buffer[REC_PREAMBLE_WIDTH];
        print_preamble(preamble_buffer, sizeof(preamble_buffer), _file, _line);
        printf("%s%s\n", preamble_buffer, _ss.str().c_str());
    };

    template <typename T>
    reconsole &operator<<(T &&t)
    {
        _ss << t;
        return *this;
    }

    // std::endl and other iomanip:s.
    reconsole &operator<<(std::ostream &(*f)(std::ostream &))
    {
        f(_ss);
        return *this;
    }

private:
    const char *_file;
    unsigned _line;
    std::ostringstream _ss;
};

struct GFunc_out;

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
};

struct GFunc_out
{
private:
    recfile &out;

public:
    GFunc_out(recfile &_os) : out(_os)
    {
    }
    template <typename Name, typename Valu>
    void operator()(Name &&fdname, Valu &&value)
    {
        out.serializeObj(value, fdname);
    }
};

#endif