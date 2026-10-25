#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace logfile {

static FILE* g_file = nullptr;

void open(const std::string& path) {
    if (g_file) fclose(g_file);
    g_file = fopen(path.c_str(), "w");
}

void write(const char* fmt, ...) {
    if (!g_file) return;
    time_t now = time(nullptr);
    tm local{};
    localtime_s(&local, &now);
    fprintf(g_file, "[%02d:%02d:%02d] ", local.tm_hour, local.tm_min, local.tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_file, fmt, args);
    va_end(args);

    fputc('\n', g_file);
    fflush(g_file);
}

} // namespace logfile
