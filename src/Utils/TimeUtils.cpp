#include "Utils/TimeUtils.hpp"

std::string get_current_timestamp()
{
    constexpr size_t ms_width = 3;
    constexpr size_t to_milliseconds = 1000;

    const std::chrono::time_point now = std::chrono::system_clock::now();
    const std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    const std::chrono::duration ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % to_milliseconds;
    struct tm timeinfo;

    std::ostringstream string_stream;
    (void)localtime_s(&timeinfo, &time_t_now);
    string_stream << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(ms_width) << ms.count();
    return string_stream.str();
}
