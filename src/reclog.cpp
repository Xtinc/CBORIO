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
std::atomic_int RECLOG::RECONFIG::cnt{0};
bool RECLOG::RECONFIG::g_compress{false};
std::atomic_size_t RECLOG::RECONFIG::filesize{0};
std::string RECLOG::RECONFIG::rootname{""};
std::vector<std::string> RECLOG::RECONFIG::pathlist{};
RECLOG::FilePtr RECLOG::RECONFIG::g_fp = std::make_shared<RECLOG::FileBase>();
RECLOG::FilePtr RECLOG::RECONFIG::g_net = std::make_shared<RECLOG::FileBase>();
FunctionPool RECLOG::RECONFIG::g_copool(2);
FunctionPool RECLOG::RECONFIG::g_expool(1);

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
            };
        }
    };

    size_t WriteData(const void *src, size_t ele_size, size_t len) override
    {
        auto bytes_writed = m_fp == nullptr ? 0 : fwrite(src, ele_size, len, m_fp);
        RECLOG::RECONFIG::filesize += bytes_writed;
        return bytes_writed;
    }

private:
    FILE *m_fp;
    std::string m_filename;
};

class FileNet : public RECLOG::FileBase, public std::enable_shared_from_this<FileNet>
{
public:
    FileNet(const std::string &IPAdresser)
    {
        m_IPAdresser = IPAdresser;
        m_buf.reserve(4096);
    };
    ~FileNet() {}

    size_t WriteData(const void *src, size_t ele_size, size_t len) override
    {
        std::vector<char> content;
        for (size_t idx = 0; idx < ele_size * len; ++idx)
        {
            content.emplace_back(*((char *)(src) + idx));
        }
        // RECLOG(log) << content;
        RECLOG::RECONFIG::g_expool.post([content, self = shared_from_this()]()
                                        { 
                                            for(auto &i:content){
                                                self->m_buf.emplace_back(i);
                                            }
                                            if(self->m_buf.size()>100000){
                                                self->m_buf.clear();
                                                RECLOG(log)<<"send data!";
                                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                            } });
        return 0;
    }

private:
    std::string m_IPAdresser;
    std::vector<char> m_buf;
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
    RECLOG(log) << "REC caught a signal: " << signal_name << "\n"
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
    // RECLOG(log) << "atexit";
}

void InitIMPL(const char *filename, bool Compressed, RECLOG::FilePtr &fp, RECLOG::FilePtr &net)
{
#if __GNUC__
    install_signal_handlers();
#endif
    RECLOG::RECONFIG::start_time = get_date_time();
    if (strlen(filename) != 0)
    {
        RECLOG::RECONFIG::filesize = 0;
        RECLOG::RECONFIG::g_compress = Compressed;
        RECLOG::RECONFIG::rootname = filename;
        RECLOG::RECONFIG::pathlist.emplace_back(RECLOG::RECONFIG::rootname + print_date_time(get_date_time()));
        fp = std::make_shared<FileDisk>(RECLOG::RECONFIG::pathlist.at(RECLOG::RECONFIG::cnt));
        net = std::make_shared<FileNet>("127.0.0.1");
    }
    else
    {
        print_header();
    }
    atexit(ExitIMPL);
}

void GenerateNewFile(RECLOG::FilePtr &fp)
{
    int old_value = RECLOG::RECONFIG::cnt.load();
    int new_value = 0;
    do
    {
        if (old_value < REC_MAX_FILENUM)
        {
            new_value = old_value + 1;
        }
        else
        {
            new_value = 0;
        }

    } while (!RECLOG::RECONFIG::cnt.compare_exchange_weak(old_value, new_value));
    RECLOG::RECONFIG::pathlist.emplace_back(RECLOG::RECONFIG::rootname + print_date_time(get_date_time()));
    fp.reset(new FileDisk(RECLOG::RECONFIG::pathlist.at(RECLOG::RECONFIG::cnt)));
}

RECLOG::FilePtr &RECLOG::RECONFIG::GetCurFileFp()
{
    size_t old_value = RECLOG::RECONFIG::filesize.load();
    bool size_reset = false;
    do
    {
        if (old_value < REC_MAX_FILESIZE)
        {
            break;
        }
        size_reset = RECLOG::RECONFIG::filesize.compare_exchange_weak(old_value, 0);
        if (size_reset)
        {
            std::call_once(RECFileFlag[RECLOG::RECONFIG::cnt], GenerateNewFile, std::ref(RECLOG::RECONFIG::g_fp));
        }
    } while (!size_reset);
    /*
        if (RECLOG::RECONFIG::filesize > REC_MAX_FILESIZE)
        {
            RECLOG::RECONFIG::filesize = 0;
            std::call_once(RECFileFlag[RECLOG::RECONFIG::cnt], GenerateNewFile, std::ref(RECLOG::RECONFIG::g_fp));
        }*/
    return RECLOG::RECONFIG::g_fp;
}

RECLOG::FilePtr &RECLOG::RECONFIG::GetCurNetFp()
{
    return RECLOG::RECONFIG::g_net;
}

void RECLOG::RECONFIG::InitREC(const char *RootName, bool Compressed)
{
    std::call_once(RECInitFlag, InitIMPL, RootName, Compressed,
                   std::ref(RECLOG::RECONFIG::g_fp), std::ref(RECLOG::RECONFIG::g_net));
}

RECLOG::RecLogger_raw::~RecLogger_raw()
{
    m_pFile->WriteData(m_ss.str().c_str(), sizeof(char), m_ss.str().size());
    // RECLOG::RECONFIG::filesize += bytes_writed;
}

RECLOG::RecLogger_file::~RecLogger_file()
{
    en << get_date_time() << get_thread_name();
    m_pFile->WriteData(m_buf.data(), sizeof(unsigned char), m_buf.size());
    // RECLOG::RECONFIG::filesize += bytes_writed;
}

RECLOG::RecLogger_log::~RecLogger_log()
{
    char preamble_buffer[REC_PREAMBLE_WIDTH];
    print_preamble(preamble_buffer, sizeof(preamble_buffer), m_verbosity, _file, _line);
    printf("%s%s\n", preamble_buffer, m_ss.str().c_str());
}