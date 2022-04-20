#include "cbor_file.h"
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

bool recfile::open(const char *filename)
{
    mpFile = fopen(filename, "wb");
    m_open = mpFile == nullptr ? false : true;
    return m_open;
}

void recfile::close()
{
    if (m_open)
    {
        fclose(mpFile);
        mpFile = nullptr;
    }
}

void recfile::write_to_disk(){
    
}