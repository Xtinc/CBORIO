#include "decoder.h"
#include <limits.h>
#include <vector>

using namespace cborio;

template <>
unsigned short decoder::get_data()
{
    unsigned short value =
        static_cast<unsigned short>(m_input.get_byte()) << 8 |
        static_cast<unsigned short>(m_input.get_byte());
    return value;
}
template <>
unsigned int decoder::get_data()
{
    unsigned int value =
        (static_cast<unsigned int>(m_input.get_byte()) << 24) |
        (static_cast<unsigned int>(m_input.get_byte()) << 16) |
        (static_cast<unsigned int>(m_input.get_byte()) << 8) |
        (static_cast<unsigned int>(m_input.get_byte()));
    return value;
}
template <>
unsigned long long decoder::get_data()
{
    unsigned long long value =
        (static_cast<unsigned long long>(m_input.get_byte()) << 56) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 48) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 40) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 32) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 24) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 16) |
        (static_cast<unsigned long long>(m_input.get_byte()) << 8) |
        (static_cast<unsigned long long>(m_input.get_byte()));
    return value;
}
template <>
float decoder::get_data()
{
    // todo: related to byte_order
    float tmp = 0.0f;
    uint8_t value[4] = {0};
    value[3] = m_input.get_byte();
    value[2] = m_input.get_byte();
    value[1] = m_input.get_byte();
    value[0] = m_input.get_byte();
    memcpy(&tmp, &value[0], sizeof(float));
    return tmp;
}
template <>
double decoder::get_data()
{
    double tmp = 0.0;
    uint8_t value[8] = {0};
    value[7] = m_input.get_byte();
    value[6] = m_input.get_byte();
    value[5] = m_input.get_byte();
    value[4] = m_input.get_byte();
    value[3] = m_input.get_byte();
    value[2] = m_input.get_byte();
    value[1] = m_input.get_byte();
    value[0] = m_input.get_byte();
    memcpy(&tmp, &value[0], sizeof(double));
    return tmp;
}

void decoder::run()
{
    unsigned int temp = 0;
    bool loop = true;
    do
    {
        switch (m_status)
        {
        case DECODER_STATUS::STATE_TYPE:
            if (m_input.has_bytes(1))
            {
                unsigned char type = m_input.get_byte();
                unsigned char major_type = type >> 5;
                unsigned char minor_type = static_cast<unsigned char>(type & 0x1f);
                switch (major_type)
                {
                case 0: // positive
                    if (minor_type <= 0x17)
                    {
                        m_handler.on_integer(minor_type);
                    }
                    else if (minor_type == 0x18)
                    { // 1 byte
                        m_curlen = 1;
                        m_status = DECODER_STATUS::STATE_PINT;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_PINT;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_PINT;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_PINT;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid integer type");
                    }
                    break;
                case 1: // negative
                    if (minor_type <= 0x17)
                    {
                        m_handler.on_integer(-1 - minor_type);
                    }
                    else if (minor_type == 0x18)
                    { // 1 byte
                        m_curlen = 1;
                        m_status = DECODER_STATUS::STATE_NINT;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_NINT;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_NINT;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_NINT;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid integer type");
                    }
                    break;
                case 2: // bytes
                    if (minor_type <= 0x17)
                    {
                        m_status = DECODER_STATUS::STATE_BYTES_DATA;
                        m_curlen = minor_type;
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_BYTES_SIZE;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_BYTES_SIZE;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_BYTES_SIZE;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_BYTES_SIZE;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid bytes type");
                    }
                    break;
                case 3: // string
                    if (minor_type <= 0x17)
                    {
                        m_status = DECODER_STATUS::STATE_STRING_DATA;
                        m_curlen = minor_type;
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_STRING_SIZE;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_STRING_SIZE;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_STRING_SIZE;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_STRING_SIZE;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid string type");
                    }
                    break;
                case 4: // array
                    if (minor_type <= 0x17)
                    {
                        m_handler.on_array(minor_type);
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_ARRAY;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_ARRAY;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_ARRAY;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_ARRAY;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid array type");
                    }
                    break;
                case 5: // map
                    if (minor_type <= 0x17)
                    {
                        m_handler.on_map(minor_type);
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_MAP;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_MAP;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_MAP;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_MAP;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid array type");
                    }
                    break;
                case 6: // tag
                    if (minor_type <= 0x17)
                    {
                        m_handler.on_tag(minor_type);
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_TAG;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_TAG;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_TAG;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_TAG;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid tag type");
                    }
                    break;
                case 7: // special
                    if (minor_type < 20)
                    {
                        m_handler.on_special(minor_type);
                    }
                    else if (minor_type == 0x14)
                    {
                        m_handler.on_bool(false);
                    }
                    else if (minor_type == 0x15)
                    {
                        m_handler.on_bool(true);
                    }
                    else if (minor_type == 0x16)
                    {
                        m_handler.on_null();
                    }
                    else if (minor_type == 0x17)
                    {
                        m_handler.on_undefined();
                    }
                    else if (minor_type == 0x18)
                    {
                        m_status = DECODER_STATUS::STATE_SPECIAL;
                        m_curlen = 1;
                    }
                    else if (minor_type == 0x19)
                    { // 2 byte
                        m_curlen = 2;
                        m_status = DECODER_STATUS::STATE_SPECIAL;
                    }
                    else if (minor_type == 0x1A)
                    { // 4 byte
                        m_curlen = 4;
                        m_status = DECODER_STATUS::STATE_FLOAT;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_DOUBLE;
                    }
                    else
                    {
                        m_status = DECODER_STATUS::STATE_ERROR;
                        m_handler.on_error("invalid special type");
                    }
                    break;
                default:
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_PINT:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                // u8
                case 1:
                    m_handler.on_integer(m_input.get_byte());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                // u16
                case 2:
                    m_handler.on_integer(get_data<unsigned short>());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                // u32
                case 4:
                    // todo :overflow
                    temp = get_data<unsigned int>();
                    if (temp <= INT_MAX)
                    {
                        m_handler.on_integer(temp);
                    }
                    else
                    {
                        m_handler.on_extra_integer(temp, 1);
                    }
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                // u64
                case 8:
                    m_handler.on_extra_integer(get_data<unsigned long long>(), 1);
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_NINT:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                case 1:
                    m_handler.on_integer(-static_cast<int>(m_input.get_byte()) - 1);
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 2:
                    m_handler.on_integer(-static_cast<int>(get_data<unsigned short>()) - 1);
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 4:
                    temp = get_data<unsigned int>();
                    if (temp <= INT_MAX)
                    {
                        m_handler.on_integer(-static_cast<int>(temp) - 1);
                    }
                    else
                    {
                        m_handler.on_extra_integer(temp + 1, -1);
                    }
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 8:
                    m_handler.on_extra_integer(get_data<unsigned long long>() + 1, -1);
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_FLOAT:
            if (m_input.has_bytes(m_curlen))
            {
                m_status = DECODER_STATUS::STATE_TYPE;
                m_handler.on_float(get_data<float>());
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_DOUBLE:
            if (m_input.has_bytes(m_curlen))
            {
                m_status = DECODER_STATUS::STATE_TYPE;
                m_handler.on_double(get_data<double>());
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_BYTES_SIZE:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                case 1:
                    m_curlen = m_input.get_byte();
                    m_status = DECODER_STATUS::STATE_BYTES_DATA;
                    break;
                case 2:
                    m_curlen = get_data<unsigned short>();
                    m_status = DECODER_STATUS::STATE_BYTES_DATA;
                    break;
                case 4:
                    m_curlen = get_data<unsigned int>();
                    m_status = DECODER_STATUS::STATE_BYTES_DATA;
                    break;
                case 8:
                    m_status = DECODER_STATUS::STATE_ERROR;
                    m_handler.on_error("extra long bytes");
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_BYTES_DATA:
            if (m_input.has_bytes(m_curlen))
            {
                std::vector<unsigned char> tp(m_curlen, 0);
                m_input.get_bytes(tp.data(), m_curlen);
                m_status = DECODER_STATUS::STATE_TYPE;
                m_handler.on_bytes(tp.data(), tp.size());
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_STRING_SIZE:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                case 1:
                    m_curlen = m_input.get_byte();
                    m_status = DECODER_STATUS::STATE_STRING_DATA;
                    break;
                case 2:
                    m_curlen = get_data<unsigned short>();
                    m_status = DECODER_STATUS::STATE_STRING_DATA;
                    break;
                case 4:
                    m_curlen = get_data<unsigned int>();
                    m_status = DECODER_STATUS::STATE_STRING_DATA;
                    break;
                case 8:
                    m_status = DECODER_STATUS::STATE_ERROR;
                    m_handler.on_error("extra long array");
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_STRING_DATA:
            if (m_input.has_bytes(m_curlen))
            {
                std::vector<unsigned char> tp(m_curlen, 0);
                m_input.get_bytes(tp.data(), tp.size());
                m_status = DECODER_STATUS::STATE_TYPE;
                std::string str((const char *)tp.data(), tp.size());
                m_handler.on_string(str);
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_ARRAY:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                case 1:
                    m_handler.on_array(m_input.get_byte());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 2:
                    m_handler.on_array(m_curlen = get_data<unsigned short>());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 4:
                    m_handler.on_array(get_data<unsigned int>());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 8:
                    m_status = DECODER_STATUS::STATE_ERROR;
                    m_handler.on_error("extra long array");
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_MAP:
            if (m_input.has_bytes(m_curlen))
            {
                switch (m_curlen)
                {
                case 1:
                    m_handler.on_map(m_input.get_byte());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 2:
                    m_handler.on_map(m_curlen = get_data<unsigned short>());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 4:
                    m_handler.on_map(get_data<unsigned int>());
                    m_status = DECODER_STATUS::STATE_TYPE;
                    break;
                case 8:
                    m_status = DECODER_STATUS::STATE_ERROR;
                    m_handler.on_error("extra long map");
                    break;
                }
            }
            else
            {
                loop = false;
            }
            break;
        case DECODER_STATUS::STATE_ERROR:
            loop = false;
            break;
        default:
            loop = false;
            break;
        }
    } while (loop);
}