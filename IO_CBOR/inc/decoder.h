#ifndef CBOR_DECODER_H
#define CBOR_DECODER_H

#include <type_traits>
namespace cborio
{
    enum class DECODER_STATUS
    {
        STATE_TYPE,
        STATE_PINT,
        STATE_NINT,
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
        virtual void on_integer(int value){};

        virtual void on_bytes(unsigned char *data, int size){};

        virtual void on_string(std::string &str){};

        virtual void on_array(int size){};

        virtual void on_map(int size){};

        virtual void on_tag(unsigned int tag){};

        virtual void on_special(unsigned int code){};

        virtual void on_bool(bool){};

        virtual void on_null(){};

        virtual void on_undefined(){};

        virtual void on_error(const char *error){};

        virtual void on_extra_integer(unsigned long long value, int sign)
        {
        }

        virtual void on_extra_tag(unsigned long long tag)
        {
        }

        virtual void on_extra_special(unsigned long long tag)
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

        template <typename RT, typename std::enable_if<std::is_integral<RT>::value>::type * = nullptr>
        RT get_data() { return RT(); }
        // to do: avoid short int long.
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
        uint8_t value[4] = {0};
        value[3] = m_input.get_byte();
        value[2] = m_input.get_byte();
        value[1] = m_input.get_byte();
        value[0] = m_input.get_byte();
        return *(reinterpret_cast<float *>(&value[0]));
    }
    template <>
    double decoder::get_data()
    {
        uint8_t value[8] = {0};
        value[7] = m_input.get_byte();
        value[6] = m_input.get_byte();
        value[5] = m_input.get_byte();
        value[4] = m_input.get_byte();
        value[3] = m_input.get_byte();
        value[2] = m_input.get_byte();
        value[1] = m_input.get_byte();
        value[0] = m_input.get_byte();
        return *(reinterpret_cast<double *>(&value[0]));
    }
}
#endif