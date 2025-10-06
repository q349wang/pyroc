#pragma once

#include <cstdarg>

namespace pyroc::util
{

enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Fatal,
};

void log(const char* fmt, va_list args);

void logCategory(LogLevel level, const char* file, const char* line, const char* message, ...)
    __attribute__((__format__(__printf__, 4, 5)));
}  // namespace pyroc::util

#define LOG_DEBUG(message, ...)                                      \
    pyroc::util::logCategory(pyroc::util::LogLevel::Debug, __FILE__, \
                             std::to_string(__LINE__).c_str(), message, ##__VA_ARGS__)
