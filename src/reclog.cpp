#include "reclog.h"
#include "reclog_impl.h"

#if __GNUC__
#include <cxxabi.h>   // for __cxa_demangle
#include <dlfcn.h>    // for dladdr
#include <execinfo.h> // for backtrace
#include <signal.h>   // for catch signals
#include <unistd.h>   // for getpid
#endif

FILE *RECLOG::RECONFIG::fp{nullptr};
long long RECLOG::RECONFIG::start_time{0};
std::atomic_size_t RECLOG::RECONFIG::filesize{0};
std::string RECLOG::RECONFIG::filename = "";
int RECLOG::RECONFIG::cnt = 0;

std::once_flag rec_init_flag;
std::once_flag rec_exit_flag;
std::once_flag rec_file_flag[details::REC_MAX_FILENUM];

#if __GNUC__
std::recursive_mutex s_mutex;

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

void flush()
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);
    fflush(stdout);
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

    printf("\n");
    printf("REC caught a signal: ");
    printf("%s", signal_name);
    printf("\n");

    // --------------------------------------------------------------------
    //      WARNING: FROM NOW ANY OPERATIONS IN USR SPACE IS NOT SAFE
    // --------------------------------------------------------------------

    if (true)
    {
        flush();
        try
        {
            RECLOG(log) << "Signal: " << signal_name;
        }
        catch (...)
        {
            // This can happed due to s_fatal_handler.
            printf("Exception caught and ignored by signal handler.\n");
        }
        flush();

        // --------------------------------------------------------------------
    }

    call_default_signal_handler(signal_number);
}

void details::install_signal_handlers()
{
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags |= SA_SIGINFO;
    sig_action.sa_sigaction = &signal_handler;

    if (sigaction(SIGABRT, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGABRT";
    }
    if (sigaction(SIGBUS, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGBUS";
    }
    if (sigaction(SIGFPE, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGFPE";
    }
    if (sigaction(SIGILL, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGILL";
    }
    if (sigaction(SIGINT, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGINT";
    }
    if (sigaction(SIGSEGV, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGSEGV";
    }
    if (sigaction(SIGTERM, &sig_action, NULL) != -1)
    {
        RECLOG(log) << "Failed to install handler for SIGTERM";
    }
}
#endif

void RECLOG::INIT_REC(const char *filename)
{
    std::call_once(rec_init_flag, details::init_impl, filename);
}

void RECLOG::EXIT_REC()
{
    std::call_once(rec_exit_flag, details::exit_impl);
}

RECLOG::reclogger_raw::~reclogger_raw()
{
    if (m_pFile != nullptr)
    {
        auto bytes_writed = fwrite(m_ss.str().c_str(), sizeof(char), m_ss.str().size(), m_pFile);
        RECLOG::RECONFIG::filesize += bytes_writed;
        if (RECLOG::RECONFIG::cnt < details::REC_MAX_FILENUM && RECLOG::RECONFIG::filesize > details::REC_MAX_FILESIZE)
        {
            std::call_once(rec_file_flag[RECLOG::RECONFIG::cnt], details::generate_newfile);
        }
    }
}

RECLOG::reclogger_file::~reclogger_file()
{
    if (m_pFile != nullptr)
    {
        en << details::get_date_time() << details::get_thread_name();
        auto bytes_writed = fwrite(m_buf.data(), sizeof(unsigned char), m_buf.size(), m_pFile);
        RECLOG::RECONFIG::filesize += bytes_writed;
        if (RECLOG::RECONFIG::filesize > details::REC_MAX_FILESIZE && RECLOG::RECONFIG::cnt < details::REC_MAX_FILENUM)
        {
            std::call_once(rec_file_flag[RECLOG::RECONFIG::cnt], details::generate_newfile);
        }
    }
    else
    {
        RECLOG(log) << "error";
    }
}

RECLOG::reclogger_log::~reclogger_log()
{
    char preamble_buffer[details::REC_PREAMBLE_WIDTH];
    details::print_preamble(preamble_buffer, sizeof(preamble_buffer), m_verbosity, _file, _line);
    printf("%s%s\n", preamble_buffer, m_ss.str().c_str());
}