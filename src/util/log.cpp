#include "util/log.h"

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

namespace pyroc::util
{

constexpr uint32_t kLogBufferSize = 1024;

constexpr std::array<const char*, 5> kLogLevelNames
    = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

void logCategory(LogLevel level, const char* file, const char* line, const char* message, ...)
{
    char buffer[kLogBufferSize];
    int32_t n = std::snprintf(buffer, sizeof(buffer), "[%s][%s:%s] %s\n",
                              kLogLevelNames[static_cast<uint32_t>(level)], file, line, message);

    if (static_cast<uint32_t>(n) >= kLogBufferSize)
    {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    va_list args;
    va_start(args, message);
    log(buffer, args);
    va_end(args);
}

void log(const char* fmt, va_list args) { std::vprintf(fmt, args); }
}  // namespace pyroc::util
