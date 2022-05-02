#include "reclog.h"
#include "reclog_impl.h"
#include <fstream>

#if __GNUC__
#include <cxxabi.h>   // for __cxa_demangle
#include <dlfcn.h>    // for dladdr
#include <execinfo.h> // for backtrace
#include <signal.h>   // for catch signals
#include <unistd.h>   // for getpid
#endif

long long RECLOG::RECONFIG::start_time{0};
int RECLOG::RECONFIG::cnt{0};
bool RECLOG::RECONFIG::g_compress{false};
std::atomic_size_t RECLOG::RECONFIG::filesize{0};
std::string RECLOG::RECONFIG::filename{""};
RECLOG::FilePtr RECLOG::RECONFIG::g_fp = std::make_shared<RECLOG::FileBase>();
FunctionPool RECLOG::RECONFIG::g_funcpool{};

std::once_flag RECInitFlag;
std::once_flag RECFileFlag[REC_MAX_FILENUM];

class FileDisk : public RECLOG::FileBase
{
public:
    FileDisk(const std::string &filename)
    {
        m_filename = filename;
        m_fp = fopen(filename.c_str(), "wb");
    };

    ~FileDisk()
    {
        if (m_fp != nullptr)
        {
            if (fclose(m_fp) == 0 && RECLOG::RECONFIG::g_compress)
            {
                RECLOG::RECONFIG::g_funcpool.post(
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
            };
        }
    };

    size_t WriteData(const void *src, size_t ele_size, size_t len) override
    {
        return m_fp == nullptr ? 0 : fwrite(src, ele_size, len, m_fp);
    }

private:
    FILE *m_fp;
    std::string m_filename;
};

#if __GNUC__
std::recursive_mutex s_mutex;

std::string stacktrace_as_stdstring(int skip)
{
    // From https://gist.github.com/fmela/591333
    void *callstack[128];
    const auto max_frames = sizeof(callstack) / sizeof(callstack[0]);
    int num_frames = backtrace(callstack, max_frames);
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
    fprintf(stderr, "\n");
    fprintf(stderr, "REC caught a signal: ");
    fprintf(stderr, "%s", signal_name);
    fprintf(stderr, "\n");

    // --------------------------------------------------------------------
    //      WARNING: FROM NOW ANY OPERATIONS IN USR SPACE IS NOT SAFE
    // --------------------------------------------------------------------
    auto backtrace_contents = stacktrace_as_stdstring(0).c_str();
    fprintf(stderr, "%s", backtrace_contents);
    RECLOG(file) << backtrace_contents;

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
        RECLOG(log) << "Failed to install handler for SIGABRT";
    }
    if (sigaction(SIGBUS, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGBUS";
    }
    if (sigaction(SIGFPE, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGFPE";
    }
    if (sigaction(SIGILL, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGILL";
    }
    if (sigaction(SIGINT, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGINT";
    }
    if (sigaction(SIGSEGV, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGSEGV";
    }
    if (sigaction(SIGTERM, &sig_action, NULL) != 0)
    {
        RECLOG(log) << "Failed to install handler for SIGTERM";
    }
}
#endif

void ExitIMPL()
{
    RECLOG::RECONFIG::GetCurFileFp().reset();
    RECLOG(log) << "atexit";
}

void InitIMPL(const char *filename, bool Compressed, RECLOG::FilePtr &fp)
{
#if __GNUC__
    install_signal_handlers();
#endif
    RECLOG::RECONFIG::start_time = get_date_time();
    if (strlen(filename) != 0)
    {
        RECLOG::RECONFIG::filesize = 0;
        RECLOG::RECONFIG::filename = std::string(filename);
        RECLOG::RECONFIG::g_compress = Compressed;
        fp = std::make_shared<FileDisk>(RECLOG::RECONFIG::filename + std::to_string(RECLOG::RECONFIG::cnt));
    }
    else
    {
        print_header();
    }
    atexit(ExitIMPL);
}

void GenerateNewFile(RECLOG::FilePtr &fp)
{
    RECLOG::RECONFIG::filesize = 0;
    ++RECLOG::RECONFIG::cnt;
    fp.reset(new FileDisk(RECLOG::RECONFIG::filename + std::to_string(RECLOG::RECONFIG::cnt)));
}

RECLOG::FilePtr &RECLOG::RECONFIG::GetCurFileFp()
{
    if (RECLOG::RECONFIG::cnt < REC_MAX_FILENUM &&
        RECLOG::RECONFIG::filesize > REC_MAX_FILESIZE)
    {
        std::call_once(RECFileFlag[RECLOG::RECONFIG::cnt], GenerateNewFile, std::ref(RECLOG::RECONFIG::g_fp));
    }
    return RECLOG::RECONFIG::g_fp;
}

void RECLOG::RECONFIG::InitREC(const char *RootName, bool Compressed)
{
    std::call_once(RECInitFlag, InitIMPL, RootName, Compressed, std::ref(RECLOG::RECONFIG::g_fp));
}

RECLOG::RecLogger_raw::~RecLogger_raw()
{
    auto bytes_writed = m_pFile->WriteData(m_ss.str().c_str(), sizeof(char), m_ss.str().size());
    RECLOG::RECONFIG::filesize += bytes_writed;
}

RECLOG::RecLogger_file::~RecLogger_file()
{
    en << get_date_time() << get_thread_name();
    auto bytes_writed = m_pFile->WriteData(m_buf.data(), sizeof(unsigned char), m_buf.size());
    RECLOG::RECONFIG::filesize += bytes_writed;
}

RECLOG::RecLogger_log::~RecLogger_log()
{
    char preamble_buffer[REC_PREAMBLE_WIDTH];
    print_preamble(preamble_buffer, sizeof(preamble_buffer), m_verbosity, _file, _line);
    printf("%s%s\n", preamble_buffer, m_ss.str().c_str());
}