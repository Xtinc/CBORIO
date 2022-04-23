#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include <sstream>

#define RECVLOG_S(verbosity) RECLOG::reclogger(RECONFIG::fp, verbosity, __FILE__, __LINE__)
#define RECLOG(verbosity_name) RECVLOG_S(RECLOG::Verbosity_##verbosity_name)

namespace RECLOG
{

    enum NamedVerbosity : int
    {
        Verbosity_FATAL = -3,
        Verbosity_ERROR = -2,
        Verbosity_WARNING = -1,
        Verbosity_INFO = 0,
    };

    void INIT_REC(const char *filename = "");

    constexpr int m_buffer_size = 4096;

    struct GFunc_out;

    class reclogger
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
        unsigned int _line;
        int m_verbosity;
        std::ostringstream _ss;

    public:
        reclogger(FILE *fp, int verbosity, const char *file, unsigned line)
            : m_buf(m_buffer_size), en(m_buf), mpFile(fp), _file(file), _line(line), m_verbosity(verbosity) {}
        ~reclogger();

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger &operator<<(T &&t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger &operator<<(T &&t)
        {
            serializeObj(t);
            return *this;
        }

        // std::endl and other iomanip:s.
        reclogger &operator<<(std::ios_base &(*f)(std::ios_base &))
        {
            if (mpFile == nullptr)
            {
                f(_ss);
            }
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
    };

    struct GFunc_out
    {
    private:
        reclogger &out;

    public:
        GFunc_out(reclogger &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

}
#endif