#include "encoder.h"
using namespace cborio;
template <>
void encoder::write_data(bool value)
{
    return value ? m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x15)) : m_out.put_byte(static_cast<unsigned char>(7 << 5 | 0x14));
}