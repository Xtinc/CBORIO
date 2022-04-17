#ifndef CBOR_DECODER_H
#define CBOR_DECODER_H

#include "templates.h"

namespace cborio
{
    enum class DECODER_STATUS
    {
        STATE_TYPE,
        STATE_PINT,
        STATE_NINT,
        STATE_FLOAT,
        STATE_DOUBLE,
        STATE_BYTES_SIZE,
        STATE_BYTES_DATA,
        STATE_STRING_SIZE,
        STATE_STRING_DATA,
        STATE_ARRAY,
        STATE_MAP,
        STATE_TAG,
        STATE_SPECIAL,
        STATE_ERROR
    };

    class input
    {
    public:
        virtual bool has_bytes(int count) = 0;
        virtual unsigned char get_byte() = 0;
        virtual void get_bytes(void *to, int count) = 0;
    };

    class CBORIOHandler
    {
    public:
        virtual void on_integer(int value) = 0;

        virtual void on_float(float value) = 0;

        virtual void on_double(double value) = 0;

        virtual void on_bytes(unsigned char *data, size_t size) = 0;

        virtual void on_string(std::string &str) = 0;

        virtual void on_array(int size) = 0;

        virtual void on_map(int size) = 0;

        virtual void on_tag(unsigned int tag) = 0;

        virtual void on_special(unsigned int code) = 0;

        virtual void on_bool(bool) = 0;

        virtual void on_null() = 0;

        virtual void on_undefined() = 0;

        virtual void on_error(const char *error) = 0;

        virtual void on_extra_integer(unsigned long long, int)
        {
        }

        virtual void on_extra_tag(unsigned long long)
        {
        }

        virtual void on_extra_special(unsigned long long)
        {
        }
    };

    class decoder
    {
    private:
        input &m_input;
        CBORIOHandler &m_handler;
        DECODER_STATUS m_status;
        int m_curlen;

        template <typename RT, typename std::enable_if<is_unsigned<RT>::value>::type * = nullptr>
        RT get_data() { return RT(); }
        template <typename RT, typename std::enable_if<std::is_floating_point<RT>::value>::type * = nullptr>
        RT get_data() { return RT(); }

    public:
        decoder(input &in, CBORIOHandler &handler)
            : m_input(in),
              m_handler(handler),
              m_status(DECODER_STATUS::STATE_TYPE),
              m_curlen(0)
        {
        }
        void run();
    };
}
#endif