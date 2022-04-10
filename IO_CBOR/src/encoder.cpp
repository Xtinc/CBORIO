#include "encoder.h"
using namespace cborio;
template <>
void encoder::write_data(bool value)
{
    return value ? m_out.put_byte(static_cast<unsigned char>(0xF5)) : m_out.put_byte(static_cast<unsigned char>(0xF4));
}
template <>
void encoder::write_data(float value)
{
    char *tmp = reinterpret_cast<char *>(&value);
    m_out.put_byte(static_cast<unsigned char>(0xFA));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 3)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 2)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 1)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp)));
    return;
}
template <>
void encoder::write_data(double value)
{
    char *tmp = reinterpret_cast<char *>(&value);
    m_out.put_byte(static_cast<unsigned char>(0xFB));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 7)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 6)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 5)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 4)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 3)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 2)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp + 1)));
    m_out.put_byte(*(reinterpret_cast<uint8_t *>(tmp)));
    return;
}