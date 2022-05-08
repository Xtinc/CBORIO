#ifndef CBOR_RECLOG_H
#define CBOR_RECLOG_H

#include "simple_reflect.h"
#include "encoder.h"
#include "thread_pool.h"
#include <atomic>
#include <memory>
#include <queue>

#define RECVLOG_A(fulltype) RECLOG::make_RecLog<fulltype>(__FILE__, __LINE__)
#define RECLOG(type) RECVLOG_A(RECLOG::Codec_##type)
#define RECVLOG_B(fulltype) RECLOG::make_RecFile<fulltype>(__FILE__, __LINE__)
#define RECFILE(type) RECVLOG_B(RECLOG::Codec_##type)

namespace RECLOG
{
    enum class CodeType
    {
        CBOR,
        RAW,
        LOG
    };

    class FileBase
    {
    public:
        FileBase(){};
        virtual ~FileBase(){};
        virtual void SetTemp(bool){};
        virtual size_t WriteData(const std::string &)
        {
            return 0;
        };
        virtual size_t WriteData(const cborio::ustring &)
        {
            return 0;
        };
        virtual size_t WriteData(const void *, size_t, size_t)
        {
            return 0;
        };

    private:
    };

    using FilePtr = std::shared_ptr<FileBase>;

    class DiskFileCluster
    {
    public:
        DiskFileCluster(const char *rootname, CodeType ftype)
            : m_ftype(ftype), m_filesize(0), m_rootname(rootname) {}
        FilePtr &GetCurFileFp();
        void SetRootName(const char *rtname)
        {
            m_rootname = rtname;
        }
        void AtExit()
        {
            while (!m_filelist.empty())
            {
                m_filelist.front()->SetTemp(false);
                m_filelist.pop();
            }
        }
        void IncraeseBytes(size_t t)
        {
            m_filesize += t;
        }

    private:
        CodeType m_ftype;
        std::queue<FilePtr> m_filelist;
        std::string m_rootname;
        std::atomic_size_t m_filesize;
        std::once_flag m_fg;

        std::string GetCurFileName();
    };

    class RECONFIG
    {
    public:
        static bool g_compress;
        static long long start_time;
        static FunctionPool g_copool;
        static FunctionPool g_expool;
        static DiskFileCluster g_flist_raw;
        static DiskFileCluster g_flist_log;
        static DiskFileCluster g_flist_cbor;
        static FilePtr &GetCurLogFp();
        static void InitREC(const char *filename = "", bool compressed = false);
        static void ExitREC();

    private:
        static FilePtr screenfile;
    };

    struct fLambdaFile;
    struct fLambdaLog;

    class Codec_RAW
    {
    private:
        std::ostringstream m_ss;
        FilePtr m_pFile;
        bool m_counted;

    public:
        Codec_RAW(FilePtr fp, bool counted) : m_pFile(fp), m_counted(counted) {}
        ~Codec_RAW();

        Codec_RAW(Codec_RAW &&other)
            : m_ss(std::move(other.m_ss)), m_pFile(std::move(other.m_pFile)) {}

        Codec_RAW &operator<<(const char *v)
        {
            m_ss.write(v, strlen(v));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        Codec_RAW &operator<<(const T &v)
        {
            m_ss.write((char *)&v, sizeof(T));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<typename T::value_type>::value, bool>::type = true>
        Codec_RAW &operator<<(const T &v)
        {
            m_ss.write((char *)v.data(), v.size() * sizeof(typename T::value_type));
            return *this;
        }

        template <typename _InputIterator>
        Codec_RAW &write(_InputIterator first, _InputIterator last)
        {
            char *data = (char *)&(*first);
            auto n = std::distance(first, last);
            m_ss.write(data, n * sizeof(*first));
            return *this;
        }

        template <typename T,
                  typename std::enable_if<std::is_fundamental<T>::value, bool>::type = true>
        Codec_RAW &write(const T *v, std::streamsize count)
        {
            m_ss.write((char *)v, sizeof(T) * count);
            return *this;
        }
    };

    class Codec_CBO
    {
    private:
        cborio::cborstream cbs;
        FilePtr m_pFile;
        bool m_counted;

    public:
        Codec_CBO(FilePtr fp, bool counted)
            : m_pFile(fp), m_counted(counted) {}
        ~Codec_CBO();

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        Codec_CBO &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        Codec_CBO &operator<<(const T &t)
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
                cbs << fieldName << obj;
            }
            else
            {
                cbs << obj;
            }
        }

        template <typename T,
                  typename std::enable_if<refl::IsReflected<typename std::decay<T>::type>::value>::type * = nullptr>
        void serializeObj(const T &obj, const char *fieldName = "")
        {
            cbs << fieldName << '{';
            refl::forEach(obj, RECLOG::fLambdaFile(*this));
            cbs << '}';
        }
    };

    class Codec_STR
    {
    private:
        const char *_file;
        unsigned int _line;
        int m_verbosity;
        bool m_counted;

    private:
        std::ostringstream m_ss;
        FilePtr m_pFile;

    public:
        Codec_STR(FilePtr fp, bool counted, int verbosity, const char *file, unsigned line)
            : _file(file), _line(line), m_verbosity(verbosity), m_counted(counted), m_pFile(fp) {}
        ~Codec_STR();

        Codec_STR(Codec_STR &&other)
            : m_ss(std::move(other.m_ss)) {}

        template <typename T,
                  typename std::enable_if<refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        Codec_STR &operator<<(const T &t)
        {
            serializeObj(t.st_t, t.st_name);
            return *this;
        }

        template <typename T,
                  typename std::enable_if<!refl::is_refl_info_st<typename std::decay<T>::type>::value>::type * = nullptr>
        Codec_STR &operator<<(const T &t)
        {
            serializeObj(t);
            return *this;
        }

        // std::endl and other iomanip:s.
        Codec_STR &operator<<(std::ios_base &(*f)(std::ios_base &))
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
        Codec_CBO &out;

    public:
        fLambdaFile(Codec_CBO &_os) : out(_os)
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
        Codec_STR &out;

    public:
        fLambdaLog(Codec_STR &_os) : out(_os)
        {
        }
        template <typename Name, typename Valu>
        void operator()(Name &&fdname, Valu &&value)
        {
            out.serializeObj(value, fdname);
        }
    };

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_RAW>::value, Codec_RAW>::type
    make_RecLog(const char *, unsigned)
    {
        return Codec_RAW(RECONFIG::GetCurLogFp(), false);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_CBO>::value, Codec_CBO>::type
    make_RecLog(const char *, unsigned)
    {
        return Codec_CBO(RECONFIG::GetCurLogFp(), false);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_STR>::value, Codec_STR>::type
    make_RecLog(const char *file, unsigned line)
    {
        return Codec_STR(RECLOG::RECONFIG::GetCurLogFp(), false, 0, file, line);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_STR>::value, Codec_STR>::type
    make_RecFile(const char *file, unsigned line)
    {
        return Codec_STR(RECONFIG::g_flist_log.GetCurFileFp(), true, 0, file, line);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_RAW>::value, Codec_RAW>::type
    make_RecFile(const char *, unsigned)
    {
        return Codec_RAW(RECONFIG::g_flist_raw.GetCurFileFp(), true);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, Codec_CBO>::value, Codec_CBO>::type
    make_RecFile(const char *, unsigned)
    {
        return Codec_CBO(RECONFIG::g_flist_cbor.GetCurFileFp(), true);
    }
}

#endif