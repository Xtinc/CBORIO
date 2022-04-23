#include "reclog.h"
#include "utilities.h"

void INIT_REC()
{
    std::call_once(rec_init_flag, init_impl);
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
};