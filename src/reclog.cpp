#include "reclog.h"
#include "reclog_impl.h"
#include <fstream>
#include <chrono>

#if __GNUC__
#include <cxxabi.h>   // for __cxa_demangle
#include <dlfcn.h>    // for dladdr
#include <execinfo.h> // for backtrace
#include <signal.h>   // for catch signals
#include <unistd.h>   // for getpid
#endif

long long RECLOG::RECONFIG::start_time{0};
bool RECLOG::RECONFIG::g_compress{false};
RECLOG::FilePtr RECLOG::RECONFIG::screenfile{new FileBase()};
FunctionPool RECLOG::RECONFIG::g_copool(2);
FunctionPool RECLOG::RECONFIG::g_expool(1);
RECLOG::DiskFileCluster RECLOG::RECONFIG::g_flist_cbor("", RECLOG::CodeType::CBOR);
RECLOG::DiskFileCluster RECLOG::RECONFIG::g_flist_raw("", RECLOG::CodeType::RAW);
RECLOG::DiskFileCluster RECLOG::RECONFIG::g_flist_log("", RECLOG::CodeType::LOG);

std::once_flag RECInitFlag;

class FileScreen : public RECLOG::FileBase
{
public:
    size_t WriteData(const void *src, size_t ele, size_t len) override
    {
        for (size_t i = 0; i < ele * len; ++i)
        {
            printf("%02X", *((char *)src + i));
        }
        printf("\n");
        fflush(stdout);
        return ele * len;
    }
    
    size_t WriteData(const std::string &str) override
    {
        return fprintf(stdout, "%s\n", str.c_str());
    };
    
    size_t WriteData(const cborio::ustring &str) override
    {
        for (auto iter = str.cbegin(); iter != str.cend(); ++iter)
        {
            printf("%02X", *iter);
        }
        printf("\n");
        fflush(stdout);
        return str.size();
    }
};

class FileDisk : public RECLOG::FileBase
{
public:
    FileDisk(const std::string &filename)
    {
        m_tempfile = true;
        m_filename = filename;
        m_fp = fopen(filename.c_str(), "wb");
    };

    ~FileDisk()
    {
        if (m_fp != nullptr && fclose(m_fp) == 0)
        {
            if (RECLOG::RECONFIG::g_compress)
            {
                RECLOG::RECONFIG::g_copool.post(
                    [](std::string str)
                    {
                        std::ifstream ifs(str, std::ios_base::binary);
                        std::ofstream ofs(str + ".cpr", std::ios_base::binary);
                        cborio::compress(ifs, ofs);
                        ifs.close();
                        ofs.close();
                        remove(str.c_str());
                    },
                    m_filename);
            }
            else
            {
                if (m_tempfile)
                {
                    remove(m_filename.c_str());
                }
            }
        }
    };

    size_t WriteData(const void *src, size_t ele_size, size_t len) override
    {
        return m_fp == nullptr ? 0 : fwrite(src, ele_size, len, m_fp);
    }

    size_t WriteData(const std::string &str) override
    {
        return m_fp == nullptr ? 0 : fwrite(str.c_str(), sizeof(char), str.size(), m_fp);
    }

    size_t WriteData(const cborio::ustring &ustr) override
    {
        return m_fp == nullptr ? 0 : fwrite(ustr.data(), sizeof(unsigned char), ustr.size(), m_fp);
    }

    void Mark(bool temp) override
    {
        m_tempfile = temp;
    }

private:
    bool m_tempfile;
    FILE *m_fp;
    std::string m_filename;
};

#if __GNUC__

std::string
stacktrace_as_stdstring(void **callstack, int num_frames, int skip)
{
    const auto max_frames = 128;
    char **symbols = backtrace_symbols(callstack, num_frames);

    std::string result;
    // Print stack traces so the most relevant ones are written last
    // Rationale: http://yellerapp.com/posts/2015-01-22-upside-down-stacktraces.html
    for (int i = num_frames - 1; i >= skip; --i)
    {
        char buf[1024];
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname)
        {
            char *demangled = NULL;
            int status = -1;
            if (info.dli_sname[0] == '_')
            {
                demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
            }
            snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
                     i - skip, int(2 + sizeof(void *) * 2), callstack[i],
                     status == 0 ? demangled : info.dli_sname == 0 ? symbols[i]
                                                                   : info.dli_sname,
                     static_cast<char *>(callstack[i]) - static_cast<char *>(info.dli_saddr));
            free(demangled);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
                     i - skip, int(2 + sizeof(void *) * 2), callstack[i], symbols[i]);
        }
        result += buf;
    }
    free(symbols);

    if (num_frames == max_frames)
    {
        result = "[truncated]\n" + result;
    }

    if (!result.empty() && result[result.size() - 1] == '\n')
    {
        result.resize(result.size() - 1);
    }

    return result;
}

void call_default_signal_handler(int signal_number)
{
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_handler = SIG_DFL;
    sigaction(signal_number, &sig_action, NULL);
    kill(getpid(), signal_number);
    // kill only emit signal not kill the program.
}

void signal_handler(int signal_number, siginfo_t *, void *)
{
    const char *signal_name = "UNKNOWN SIGNAL";

    if (signal_number == SIGABRT)
    {
        signal_name = "SIGABRT";
    }
    else if (signal_number == SIGBUS)
    {
        signal_name = "SIGBUS";
    }
    else if (signal_number == SIGFPE)
    {
        signal_name = "SIGFPE";
    }
    else if (signal_number == SIGILL)
    {
        signal_name = "SIGILL";
    }
    else if (signal_number == SIGINT)
    {
        signal_name = "SIGINT";
    }
    else if (signal_number == SIGSEGV)
    {
        signal_name = "SIGSEGV";
    }
    else if (signal_number == SIGTERM)
    {
        signal_name = "SIGTERM";
    }
    else
    {
        signal_name = "UNKNOWN SIGNAL";
    }

    // --------------------------------------------------------------------
    //      WARNING: FROM NOW ANY OPERATIONS IN USR SPACE IS NOT SAFE
    // --------------------------------------------------------------------

    void *callstack[128];
    const auto max_frames = sizeof(callstack) / sizeof(callstack[0]);
    int num_frames = backtrace(callstack, max_frames);
    // guaranteed on most situations.
    char err_file[32] = {0};
    snprintf(err_file, 32, "%d.crash", getpid());
    FILE *err_fd = fopen(err_file, "w");
    backtrace_symbols_fd(callstack, num_frames, fileno(err_fd));
    // guaranteed on most situations.
    auto err_pretty_message = stacktrace_as_stdstring(callstack, num_frames, 0);
    fprintf(err_fd, "%s", "********Readable MSGS:\n");
    fprintf(err_fd, "%s", err_pretty_message.c_str());
    // guaranteed on many situations.
    RECLOG(STR) << "REC caught a signal: " << signal_name << "\n"
                << err_pretty_message;
    // guaranteed on some situations.
    fclose(err_fd);
    // fprintf(stderr, "%s", stacktrace_as_stdstring(0).c_str());
    call_default_signal_handler(signal_number);
}

void install_signal_handlers()
{
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags |= SA_SIGINFO;
    sig_action.sa_sigaction = &signal_handler;

    if (sigaction(SIGABRT, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGABRT";
    }
    if (sigaction(SIGBUS, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGBUS";
    }
    if (sigaction(SIGFPE, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGFPE";
    }
    if (sigaction(SIGILL, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGILL";
    }
    if (sigaction(SIGINT, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGINT";
    }
    if (sigaction(SIGSEGV, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGSEGV";
    }
    if (sigaction(SIGTERM, &sig_action, NULL) != 0)
    {
        RECLOG(STR) << "Failed to install handler for SIGTERM";
    }
}
#endif

std::string RECLOG::DiskFileCluster::GetCurFileName()
{
    std::string filename = m_rootname + print_date_time(get_date_time());
    switch (m_ftype)
    {
    case CodeType::CBOR:
        filename += ".cbor";
        break;
    case CodeType::LOG:
        filename += ".log";
        break;
    case CodeType::RAW:
        filename += ".dat";
        break;
    default:
        break;
    };
    return filename;
}

RECLOG::FilePtr &RECLOG::DiskFileCluster::GetCurFileFp()
{
    std::call_once(m_fg, [&]()
                   { m_filelist.push(std::move(std::make_shared<FileDisk>(GetCurFileName()))); });
    size_t old_value = m_filesize.load();
    bool size_reset = false;
    do
    {
        if (old_value < REC_MAX_FILESIZE)
        {
            break;
        }
        size_reset = m_filesize.compare_exchange_weak(old_value, 0);
        if (size_reset)
        {
            m_filelist.push(std::move(std::make_shared<FileDisk>(GetCurFileName())));
            if (m_filelist.size() > REC_MAX_FILENUM)
            {
                m_filelist.pop();
            }
        }
    } while (!size_reset);
    return m_filelist.back();
}

RECLOG::FilePtr &RECLOG::RECONFIG::GetCurLogFp()
{
    return RECLOG::RECONFIG::screenfile;
}

void InitIMPL(const char *filename, bool Compressed)
{
#if __GNUC__
    install_signal_handlers();
#endif
    RECLOG::RECONFIG::start_time = get_date_time();
    RECLOG::RECONFIG::GetCurLogFp().reset(new FileScreen());
    if (strlen(filename) != 0)
    {
        RECLOG::RECONFIG::g_compress = Compressed;
        RECLOG::RECONFIG::g_flist_cbor.SetRootName(filename);
        RECLOG::RECONFIG::g_flist_log.SetRootName(filename);
        RECLOG::RECONFIG::g_flist_raw.SetRootName(filename);
    }
    else
    {
        print_header();
    }
    atexit(RECLOG::RECONFIG::ExitREC);
}

void RECLOG::RECONFIG::InitREC(const char *RootName, bool Compressed)
{
    std::call_once(RECInitFlag, InitIMPL, RootName, Compressed);
}

void RECLOG::RECONFIG::ExitREC()
{
    RECLOG::RECONFIG::g_flist_cbor.AtExit();
    RECLOG::RECONFIG::g_flist_log.AtExit();
    RECLOG::RECONFIG::g_flist_raw.AtExit();
}

RECLOG::Codec_RAW::~Codec_RAW()
{
    auto bytes_writed = m_pFile->WriteData(m_ss.str().c_str(), sizeof(char), m_ss.str().length());
    if (m_counted)
    {
        RECLOG::RECONFIG::g_flist_raw.IncraeseBytes(bytes_writed);
    }
}

RECLOG::Codec_CBO::~Codec_CBO()
{
    cbs << get_date_time() << get_thread_name();
    auto bytes_writed = m_pFile->WriteData(cbs.u_str());
    if (m_counted)
    {
        RECLOG::RECONFIG::g_flist_cbor.IncraeseBytes(bytes_writed);
    }
}

RECLOG::Codec_STR::~Codec_STR()
{
    char preamble_buffer[REC_PREAMBLE_WIDTH];
    print_preamble(preamble_buffer, sizeof(preamble_buffer), m_verbosity, _file, _line);
    auto content = m_ss.str().insert(0, preamble_buffer);
    auto bytes_writed = m_pFile->WriteData(content);
    if (m_counted)
    {
        RECLOG::RECONFIG::g_flist_cbor.IncraeseBytes(bytes_writed);
    }
    // printf("%s%s\n", preamble_buffer, m_ss.str().c_str());
}