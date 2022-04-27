#ifndef CBOR_UTILITIES_H
#define CBOR_UTILITIES_H

#include <cstring>
#ifdef _MSC_VER
#include <direct.h>
#define localtime_r(a, b) localtime_s(b, a) // No localtime_r with MSVC, but arguments are swapped for localtime_s
#endif
#include <chrono>
#include <thread>
#include <mutex>

namespace
{
    constexpr int REC_THREADNAME_WIDTH = 8;
    constexpr int REC_FILENAME_WIDTH = 23;
    constexpr int REC_PREAMBLE_WIDTH = 54 + REC_THREADNAME_WIDTH + REC_FILENAME_WIDTH;
    std::once_flag rec_init_flag;
    std::once_flag rec_exit_flag;

    inline const char *filename(const char *path)
    {
        for (auto ptr = path; *ptr; ++ptr)
        {
            if (*ptr == '/' || *ptr == '\\')
            {
                path = ptr + 1;
            }
        }
        return path;
    }

    inline long long get_date_time()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    inline void print_date_time(long long ms_since_epoch, char *buff, size_t buff_size)
    {
        time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
        tm time_info;
        localtime_r(&sec_since_epoch, &time_info);
        snprintf(buff, buff_size, "%04d%02d%02dT%02d%02d%02d.%03lld",
                 1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
                 time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
    }

    inline unsigned int get_thread_name()
    {
        return static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }

    inline const char *get_verbosity_name(int verbosity)
    {
        const char *name = nullptr;
        if (verbosity <= -3)
        {
            name = "-3";
        }
        else if (verbosity == -2)
        {
            name = "-2";
        }
        else if (verbosity == -1)
        {
            name = "-1";
        }
        else if (verbosity == 0)
        {
            name = "0";
        }

        return name;
    }

    inline void print_thread_name(unsigned int thread_id, char *buffer, unsigned long long length, bool right_align_hex_id = true)
    {
        if (right_align_hex_id)
        {
            snprintf(buffer, static_cast<size_t>(length), "%*X", static_cast<int>(length - 1), thread_id);
        }
        else
        {
            snprintf(buffer, static_cast<size_t>(length), "%X", thread_id);
        }
    }

    inline void print_preamble_header(char *out_buff, size_t out_buff_size)
    {
        if (out_buff_size == 0)
        {
            return;
        }
        out_buff[0] = '\0';
        size_t pos = 0;
        auto update_bytes = [&pos](int bytes)
        {
            if (bytes > 0)
            {
                pos += bytes;
            }
        };
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "date       time         "));
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "(   uptime) "));
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "[%-*s]", REC_THREADNAME_WIDTH, "threadid"));
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "%*s:line   ", REC_FILENAME_WIDTH, "file"));
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "   l |"));
    }

    // focus on thread safety.
    inline void print_preamble(char *out_buff, size_t out_buff_size, int verbosity, const char *file, unsigned int line)
    {
        if (out_buff_size == 0)
        {
            return;
        }
        out_buff[0] = '\0';
        long long ms_since_epoch = get_date_time();
        time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
        tm time_info;
        localtime_r(&sec_since_epoch, &time_info);

        auto uptime_ms = ms_since_epoch - RECLOG::RECONFIG::start_time;
        auto uptime_sec = static_cast<double>(uptime_ms) / 1000.0;

        char thread_name[REC_THREADNAME_WIDTH + 1] = {0};
        print_thread_name(get_thread_name(), thread_name, REC_THREADNAME_WIDTH + 1, true);

        const char *level_buff = get_verbosity_name(verbosity);

        file = filename(file);

        size_t pos = 0;

        auto update_bytes = [&pos](int bytes)
        {
            if (bytes > 0)
            {
                pos += bytes;
            }
        };

        // date-time
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "%04d-%02d-%02d %02d:%02d:%02d.%03lld ",
                              1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
                              time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000));
        // update_time
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "(%8.3fs) ", uptime_sec));
        // thread_id
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "[%-*s]", REC_THREADNAME_WIDTH, thread_name));
        // file
        char shortened_filename[REC_FILENAME_WIDTH + 1];
        snprintf(shortened_filename, REC_FILENAME_WIDTH + 1, "%s", file);
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "%*s:%-6u ",
                              REC_FILENAME_WIDTH, shortened_filename, line));
        // level
        update_bytes(snprintf(out_buff + pos, out_buff_size - pos, "%4s | ", level_buff));
        // delimiter
        // update_bytes(snprintf(out_buff + pos, out_buff_size - pos, " | "));
    }

    inline void print_header()
    {
        char preamble_explain[REC_PREAMBLE_WIDTH];
        print_preamble_header(preamble_explain, sizeof(preamble_explain));
        printf("%s\n", preamble_explain);
    }

    inline void init_impl(const char *filename)
    {
        if (strlen(filename) != 0)
        {
            RECLOG::RECONFIG::fp = fopen(filename, "wb");
        }
        else
        {
            RECLOG::RECONFIG::start_time = get_date_time();
            print_header();
        }
    }

    inline void exit_impl()
    {
        if (RECLOG::RECONFIG::fp != nullptr)
        {
            fclose(RECLOG::RECONFIG::fp);
        }
    }
}

#endif