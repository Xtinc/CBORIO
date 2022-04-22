#include "reclog.h"
#include "utilities.h"

void recfile::set_date_thread()
{
    en << get_date_time() << get_thread_name();
}

void recfile::write_to_disk()
{
    if (getFILE())
    {
        fwrite(m_buf.data(), sizeof(unsigned char), m_buf.size(), getFILE());
    }
    m_buf.clear();
}