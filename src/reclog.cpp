#include "reclog.h"
#include "utilities.h"

FILE *RECONFIG::fp = nullptr;
long long RECONFIG::start_time = 0;

void INIT_REC(const char* filename)
{
    std::call_once(rec_init_flag, init_impl, filename);
}

reconsole::~reconsole()
{
    if (mpFile != nullptr)
    {
        en << get_date_time() << get_thread_name();
        fwrite(m_buf.data(), sizeof(unsigned char), m_buf.size(), mpFile);
    }
    else
    {
        char preamble_buffer[REC_PREAMBLE_WIDTH];
        print_preamble(preamble_buffer, sizeof(preamble_buffer), _file, _line);
        printf("%s%s\n", preamble_buffer, _ss.str().c_str());
    }
}