#ifndef MY_CLASS_H
#define MY_CLASS_H

#include <fstream>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <sstream>
#include "utilities.h"
#include "encoder.h"
#include "decoder.h"

class my_o : public cborio::output
{
private:
public:
    my_o() {}
    void put_byte(unsigned char c) override
    {
        std::cout << hex(c);
        return;
    }
    void put_bytes(const unsigned char *data, size_t size) override
    {
        for (int i = 0; i < size; ++i)
        {
            std::cout << hex(*(data + i));
        }
        return;
    }
};

class str_o : public cborio::output
{
private:
    std::string str;

public:
    void put_byte(unsigned char c) override
    {
        std::stringstream ss("");
        ss << hex(c);
        str += ss.str();
        return;
    }
    void put_bytes(const unsigned char *data, size_t size) override
    {
        std::stringstream ss("");
        for (int i = 0; i < size; ++i)
        {
            ss << hex(*(data + i));
        }
        str += ss.str();
        return;
    }
    void clear()
    {
        str = "";
    }
    const char *cstr()
    {
        return str.c_str();
    }
};

class wo_disk_file : public cborio::output, public std::ofstream
{
private:
public:
    wo_disk_file(const char *filename)
    {
        open(filename, std::ofstream::binary);
    }
    ~wo_disk_file()
    {
        close();
    }
    void put_byte(unsigned char value) override
    {
        write(reinterpret_cast<char *>(&value), sizeof(unsigned char));
    }

    void put_bytes(const unsigned char *data, size_t size) override
    {
        write(reinterpret_cast<const char *>(data), size);
    };
};

class wo_file : public cborio::output
{
private:
    std::vector<unsigned char> m_buffer;
    // on stack todo

public:
    wo_file(unsigned int capacity)
        : m_buffer(capacity, 0){};

    unsigned char *getData()
    {
        return m_buffer.data();
    };

    size_t getSize() const
    {
        return m_buffer.size();
    };

    void put_byte(unsigned char value) override
    {
        m_buffer.emplace_back(value);
    }

    void put_bytes(const unsigned char *data, size_t size) override
    {
        for (auto i = 0; i < size; ++i)
        {
            m_buffer.emplace_back(*(data + i));
        }
    };

    void clear()
    {
        m_buffer.clear();
    }
};

class ro_disk_file : public cborio::input, public std::ifstream
{
private:
    int64_t m_length;
    int64_t m_offset;

public:
    ro_disk_file(const char *filename)
        : m_length(0), m_offset(0)
    {
        open(filename, std::ifstream::binary);
        if (is_open())
        {
            seekg(0, end);
            m_length = tellg();
            seekg(0, beg);
        }
    }
    bool has_bytes(int count) override
    {
        return m_length - m_offset >= count;
    }
    unsigned char get_byte() override
    {
        unsigned char a = 0x0;
        read(reinterpret_cast<char *>(&a), sizeof(unsigned char));
        ++m_offset;
        return a;
    }
    void get_bytes(void *to, int count)
    {
        read(reinterpret_cast<char *>(to), count);
        m_offset += count;
    }
};

class ro_file : public cborio::input
{
private:
    const unsigned char *m_data;
    int64_t m_size;
    int64_t m_offset;

public:
    ro_file(const unsigned char *data, size_t size)
    {
        m_data = data;
        m_size = size;
        m_offset = 0;
    };

    bool has_bytes(int count) override
    {
        return m_size - m_offset >= count;
    }

    unsigned char get_byte() override
    {
        return m_data[m_offset++];
    }

    void get_bytes(void *to, int count)
    {
        memcpy(to, m_data + m_offset, count);
        m_offset += count;
    }
};

class hd_debug : public cborio::CBORIOHandler
{
public:
    void on_integer(int value)
    {
        printf("integer: %d\n", value);
    }

    void on_float(float value)
    {
        printf("float: %7.3f\n", value);
    }

    void on_double(double value)
    {
        printf("double: %15.7f\n", value);
    }

    void on_bytes(unsigned char *data, size_t size)
    {
        printf("bytes with size %lld: ", size);
        for (auto i = 0; i < size; ++i)
        {
            printf("%2x", *(data + i));
        }
        printf("\n");
    }

    void on_string(std::string &str)
    {
        printf("string: '%.*s'\n", (int)str.size(), str.c_str());
    }

    void on_array(int size)
    {
        printf("array: %d\n", size);
    }

    void on_map(int size)
    {
        printf("map: %d\n", size);
    }

    void on_tag(unsigned int tag)
    {
        printf("tag: %d\n", tag);
    }

    void on_special(unsigned int code)
    {
        printf("special: %d\n", code);
    }

    void on_bool(bool value)
    {
        printf("bool: %s\n", value ? "true" : "false");
    }

    void on_null()
    {
        printf("special: null\n");
    }

    void on_undefined()
    {
        printf("special: undefined\n");
    }

    void on_error(const char *error)
    {
        printf("error: %s\n", error);
    }

    void on_extra_integer(unsigned long long value, int sign)
    {
        if (sign >= 0)
        {
            printf("extra integer: %llu\n", value);
        }
        else
        {
            printf("extra integer: -%llu\n", value);
        }
    }

    void on_extra_tag(unsigned long long tag)
    {
        printf("extra tag: %llu\n", tag);
    }

    void on_extra_special(unsigned long long tag)
    {
        printf("extra special: %llu\n", tag);
    }
};

#endif