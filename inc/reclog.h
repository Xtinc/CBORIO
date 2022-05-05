#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include "thread_pool.h"
#include <atomic>
#include <memory>

#define RECVLOG_S(fulltype) RECLOG::make_RecLogger<fulltype>(__FILE__, __LINE__)
#define RECLOG(type) RECVLOG_S(RECLOG::RecLogger_##type)

namespace RECLOG
{
    class FileBase
    {
    public:
        FileBase(){};
        virtual ~FileBase(){};
        virtual size_t WriteData(const void *, size_t, size_t)
        {
            return 0;
        };

    private:
    };

    using FilePtr = std::shared_ptr<FileBase>;

    class RECONFIG
    {
    public:
        static long long start_time;
        static std::atomic_size_t filesize;
        static std::string filename;
        static int cnt;
        static FunctionPool g_copool;
        static FunctionPool g_expool;
        static bool g_compress;
        static FilePtr &GetCurFileFp();
        static FilePtr &GetCurNetFp();
        static void InitREC(const char *filename = "", bool compressed = false);

    private:
        static FilePtr g_fp;
        static FilePtr g_net;
    };

    struct fLambdaFile;
    struct fLambdaLog;

    class RecLogger_net
    {
    };
    class RecLogger_raw
    {
    private:
        std::ostringstream m_ss;
        FilePtr m_pFile;

    public:
        RecLogger_raw(FilePtr fp) : m_pFile(fp) {}
        ~RecLogger_raw();

        RecLogger_raw(RecLogger_raw &&other)
            : m_ss(std::move(other.m_ss)), m_pFile(std::move(other.m_pFile)) {}

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        RecLogger_raw &operator<<(const T &v)
        {
            m_ss.write((char *)&v, sizeof(T));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<typename T::value_type>::value, bool>::type = true>
        RecLogger_raw &operator<<(const T &v)
        {
            m_ss.write((char *)v.data(), v.size() * sizeof(typename T::value_type));
            return *this;
        }

        template <typename _InputIterator>
        RecLogger_raw &write(_InputIterator first, _InputIterator last)
        {
            char *data = (char *)&(*first);
            auto n = std::distance(first, last);
            m_ss.write(data, n * sizeof(*first));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        RecLogger_raw &write(const T *v, std::streamsize count)
        {
            m_ss.write((char *)v, sizeof(T) * count);
            return *this;
        }
    };

    class RecLogger_file
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
        FilePtr m_pFile;

    public:
        RecLogger_file(FilePtr fp)
            : m_buf(4096), en(m_buf), m_pFile(fp) {}
        ~RecLogger_file();

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        RecLogger_file &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        RecLogger_file &operator<<(const T &t)
        {
            serializeObj(t);
            return *this;
        }

    private:
        friend struct fLambdaFile;
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
            refl::forEach(obj, RECLOG::fLambdaFile(*this));
            en << '}';
        }
    };

    class RecLogger_log
    {
    private:
        const char *_file;
        unsigned int _line;
        int m_verbosity;
        std::ostringstream m_ss;

    public:
        RecLogger_log(int verbosity, const char *file, unsigned line)
            : _file(file), _line(line), m_verbosity(verbosity) {}
        ~RecLogger_log();

        RecLogger_log(RecLogger_log &&other)
            : m_ss(std::move(other.m_ss)) {}

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        RecLogger_log &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        RecLogger_log &operator<<(const T &t)
        {
            serializeObj(t);
            return *this;
        }

        // std::endl and other iomanip:s.
        RecLogger_log &operator<<(std::ios_base &(*f)(std::ios_base &))
        {
            f(m_ss);
            return *this;
        }

    private:
        friend struct fLambdaLog;
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
            refl::forEach(obj, RECLOG::fLambdaLog(*this));
            m_ss << '}';
        }
    };

    struct fLambdaFile
    {
    private:
        RecLogger_file &out;

    public:
        fLambdaFile(RecLogger_file &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

    struct fLambdaLog
    {
    private:
        RecLogger_log &out;

    public:
        fLambdaLog(RecLogger_log &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

    template <typename T>
    typename std::enable_if<std::is_same<T, RecLogger_raw>::value, RecLogger_raw>::type
    make_RecLogger(const char *, unsigned)
    {
        return RecLogger_raw(RECONFIG::GetCurFileFp());
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, RecLogger_file>::value, RecLogger_file>::type
    make_RecLogger(const char *, unsigned)
    {
        return RecLogger_file(RECONFIG::GetCurFileFp());
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, RecLogger_log>::value, RecLogger_log>::type
    make_RecLogger(const char *file, unsigned line)
    {
        return RecLogger_log(0, file, line);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, RecLogger_net>::value, RecLogger_raw>::type
    make_RecLogger(const char *file, unsigned line)
    {
        return RecLogger_raw(RECONFIG::GetCurNetFp());
    }
}

#endif