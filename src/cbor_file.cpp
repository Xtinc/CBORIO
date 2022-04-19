#include "cbor_file.h"

void cbostream::write_to_disk()
{
    if (is_open() && m_buf.size() > 1024)
    {
        write(reinterpret_cast<const char *>(m_buf.data()), m_buf.size());
        m_buf.clear();
        ++cnt;
    }
}