#include "Timer.hpp"

#include <vector>
#include <ctime>

std::string Timer::get_current_http_time()
{
    time_t now = std::time(0);
    struct tm* current_time = std::gmtime(&now);
    char current_time_string[30];
    std::strftime(current_time_string, 30, "%a, %d %b %Y %H:%M:%S GMT", current_time);
    return current_time_string;
}

void Timer::reset_start_time()
{
    start_time = clock_type::now();
}

Timer::duration_type Timer::get_elapsed_time() const
{
    return (clock_type::now() - start_time );
}

Timer::duration_type Timer::get_elapsed_time_in_millisecond() const
{
    return std::chrono::duration_cast< milliseconds_type >( get_elapsed_time() );
}

Timer::duration_type Timer::get_elapsed_time_in_second() const
{
    return std::chrono::duration_cast< seconds_type >( get_elapsed_time() );
}