#ifndef CBOR_UTILITIES_H
#define CBOR_UTILITIES_H

#include <string>
#ifdef _MSC_VER
#include <direct.h>
#define localtime_r(a, b) localtime_s(b, a) // No localtime_r with MSVC, but arguments are swapped for localtime_s
#endif
#include <chrono>
#include <thread>

long long get_date_time()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void print_date_time(long long ms_since_epoch, char *buff, size_t buff_size)
{
    time_t sec_since_epoch = time_t(ms_since_epoch / 1000);
    tm time_info;
    localtime_r(&sec_since_epoch, &time_info);
    snprintf(buff, buff_size, "%04d%02d%02dT%02d%02d%02d.%03lld",
             1900 + time_info.tm_year, 1 + time_info.tm_mon, time_info.tm_mday,
             time_info.tm_hour, time_info.tm_min, time_info.tm_sec, ms_since_epoch % 1000);
}

unsigned int get_thread_name()
{
    return static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

void print_thread_name(unsigned int thread_id, char *buffer, unsigned long long length, bool right_align_hex_id = true)
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

#endif