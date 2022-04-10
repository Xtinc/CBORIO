#include "encoder.h"
#include "decoder.h"
#include <limits.h>

using namespace cborio;

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
                        m_status = DECODER_STATUS::STATE_SPECIAL;
                    }
                    else if (minor_type == 0x1B)
                    { // 8 byte
                        m_curlen = 8;
                        m_status = DECODER_STATUS::STATE_SPECIAL;
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
                    m_handler.on_integer(-static_cast<int>(get_data<unsigned int>()) - 1);
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
            break;
        case DECODER_STATUS::STATE_BYTES_DATA:
            if (m_input.has_bytes(m_curlen))
            {
                unsigned char *data = new unsigned char[m_curlen];
                m_input.get_bytes(data, m_curlen);
                m_status = DECODER_STATUS::STATE_TYPE;
                m_handler.on_bytes(data, m_curlen);
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
            break;
        case DECODER_STATUS::STATE_STRING_DATA:
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