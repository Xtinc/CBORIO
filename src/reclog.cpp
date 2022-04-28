#include "reclog.h"
#include "reclog_impl.h"

FILE *RECLOG::RECONFIG::fp{nullptr};
long long RECLOG::RECONFIG::start_time{0};
std::atomic_size_t RECLOG::RECONFIG::filesize{0};
std::string RECLOG::RECONFIG::filename = "";
int RECLOG::RECONFIG::cnt = 0;

std::once_flag rec_file_flag[REC_MAX_FILENUM];

void RECLOG::INIT_REC(const char *filename)
{
    std::call_once(rec_init_flag, init_impl, filename);
}

void RECLOG::EXIT_REC()
{
    std::call_once(rec_exit_flag, exit_impl);
}

RECLOG::reclogger_raw::~reclogger_raw()
{
    if (m_pFile != nullptr)
    {
        auto bytes_writed = fwrite(m_ss.str().c_str(), sizeof(char), m_ss.str().size(), m_pFile);
        RECLOG::RECONFIG::filesize += bytes_writed;
        if (RECLOG::RECONFIG::cnt < REC_MAX_FILENUM && RECLOG::RECONFIG::filesize > REC_MAX_FILESIZE)
        {
            std::call_once(rec_file_flag[RECLOG::RECONFIG::cnt], generate_newfile);
        }
    }
}

RECLOG::reclogger_file::~reclogger_file()
{
    if (m_pFile != nullptr)
    {
        en << get_date_time() << get_thread_name();
        auto bytes_writed = fwrite(m_buf.data(), sizeof(unsigned char), m_buf.size(), m_pFile);
        RECLOG::RECONFIG::filesize += bytes_writed;
        if (RECLOG::RECONFIG::filesize > REC_MAX_FILESIZE && RECLOG::RECONFIG::cnt < REC_MAX_FILENUM)
        {
            std::call_once(rec_file_flag[RECLOG::RECONFIG::cnt], generate_newfile);
        }
    }
    else
    {
        RECLOG(log) << "error";
    }
}

RECLOG::reclogger_log::~reclogger_log()
{
    char preamble_buffer[REC_PREAMBLE_WIDTH];
    print_preamble(preamble_buffer, sizeof(preamble_buffer), m_verbosity, _file, _line);
    printf("%s%s\n", preamble_buffer, m_ss.str().c_str());
}