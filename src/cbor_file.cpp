#include "cbor_file.h"
#include "compress.h"

void cbostream::write_to_disk()
{
    if (is_open() && m_buf.size() > 10000)
    {
        // std::unique_ptr<unsigned char[]> tp(new unsigned char[m_buf.size() + 32]);
        // auto len = cborio::HuffmanCompress(m_buf.data(), m_buf.size(), tp.get());
        write(reinterpret_cast<const char *>(m_buf.data()), m_buf.size());
        m_buf.clear();
        ++cnt;
    }
}