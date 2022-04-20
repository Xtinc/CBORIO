#include "cbor_file.h"
#include "utilities.h"
/*
void cbostream::write_to_disk()
{
    if (is_open() && m_buf.size() > 10000)
    {
        // std::unique_ptr<unsigned char[]> tp(new unsigned char[m_buf.size() + 32]);
        // auto len = cborio::HuffmanCompress(m_buf.data(), m_buf.size(), tp.get());
        //fprintf(this)
        write(reinterpret_cast<const char *>(m_buf.data()), m_buf.size());
        m_buf.clear();
    }
}*/

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