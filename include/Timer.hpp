#ifndef TIMER_HPP
#define TIMER_HPP

#include <string>

namespace Timer
{
    /**
     * @brief  Get current time.
     * @return  Time string in Greenwich Mean Time (GMT).
     * 
     * For example: 
     * Tue, 15 Nov 1994 08:12:31 GMT
     * Wed, 26 Aug 2020 14:17:49 GMT
     */
    std::string get_current_time();
}
#endif

