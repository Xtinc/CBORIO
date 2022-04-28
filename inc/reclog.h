#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include <sstream>
#include <atomic>

#define RECVLOG_S(fulltype) RECLOG::make_reclogger<fulltype>(RECLOG::RECONFIG::fp, __FILE__, __LINE__)
#define RECLOG(type) RECVLOG_S(RECLOG::reclogger_##type)

namespace RECLOG
{
    class RECONFIG
    {
    public:
        static FILE *fp;
        static long long start_time;
        static std::atomic_size_t filesize;
        static std::string filename;
        static int cnt;
    };

    void INIT_REC(const char *filename = "");

    void EXIT_REC();

    struct f_lambda_file;
    struct f_lambda_log;

    class reclogger_raw
    {
    private:
        std::ostringstream m_ss;
        FILE *m_pFile;

    public:
        reclogger_raw(FILE *fp) : m_pFile(fp) {}
        ~reclogger_raw();

        reclogger_raw(reclogger_raw &&other)
            : m_ss(std::move(other.m_ss)), m_pFile(other.m_pFile) {}

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        reclogger_raw &operator<<(const T &v)
        {
            m_ss.write((char *)&v, sizeof(T));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<typename T::value_type>::value, bool>::type = true>
        reclogger_raw &operator<<(const T &v)
        {
            m_ss.write((char *)v.data(), v.size() * sizeof(typename T::value_type));
            return *this;
        }

        template <typename _InputIterator>
        reclogger_raw &write(_InputIterator first, _InputIterator last)
        {
            char *data = (char *)&(*first);
            auto n = std::distance(first, last);
            m_ss.write(data, n * sizeof(*first));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        reclogger_raw &write(const T *v, std::streamsize count)
        {
            m_ss.write((char *)v, sizeof(T) * count);
            return *this;
        }
    };

    class reclogger_file
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
        FILE *m_pFile;

    public:
        reclogger_file(FILE *fp)
            : m_buf(4096), en(m_buf), m_pFile(fp) {}
        ~reclogger_file();

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger_file &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger_file &operator<<(const T &t)
        {
            serializeObj(t);
            return *this;
        }

    private:
        friend struct f_lambda_file;
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
            refl::forEach(obj, f_lambda_file(*this));
            en << '}';
        }
    };

    class reclogger_log
    {
    private:
        const char *_file;
        unsigned int _line;
        int m_verbosity;
        std::ostringstream m_ss;

    public:
        reclogger_log(int verbosity, const char *file, unsigned line)
            : _file(file), _line(line), m_verbosity(verbosity) {}
        ~reclogger_log();

        reclogger_log(reclogger_log &&other)
            : m_ss(std::move(other.m_ss)) {}

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger_log &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        reclogger_log &operator<<(const T &t)
        {
            serializeObj(t);
            return *this;
        }

        // std::endl and other iomanip:s.
        reclogger_log &operator<<(std::ios_base &(*f)(std::ios_base &))
        {
            f(m_ss);
            return *this;
        }

    private:
        friend struct f_lambda_log;
        template <typename T,
                  typename std::enable_if<!refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
        void serializeObj(const T &obj, const char *fieldName = "")
        {
            if (*fieldName)
            {
                m_ss << fieldName << obj;
            }
            else
            {
                m_ss << obj;
            }
        }

        template <typename T,
                  typename std::enable_if<refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
        void serializeObj(const T &obj, const char *fieldName = "")
        {
            m_ss << fieldName << '{';
            refl::forEach(obj, f_lambda_log(*this));
            m_ss << '}';
        }
    };
    // todo : should be const T& or T& or T ? depend on restrictions.

    struct f_lambda_file
    {
    private:
        reclogger_file &out;

    public:
        f_lambda_file(reclogger_file &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

    struct f_lambda_log
    {
    private:
        reclogger_log &out;

    public:
        f_lambda_log(reclogger_log &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

    template <typename T>
    typename std::enable_if<std::is_same<T, reclogger_raw>::value, reclogger_raw>::type
    make_reclogger(FILE *fp, const char *, unsigned)
    {
        return reclogger_raw(fp);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, reclogger_file>::value, reclogger_file>::type
    make_reclogger(FILE *fp, const char *, unsigned)
    {
        return reclogger_file(fp);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, reclogger_log>::value, reclogger_log>::type
    make_reclogger(FILE *, const char *file, unsigned line)
    {
        return reclogger_log(0, file, line);
    }
}

#endif