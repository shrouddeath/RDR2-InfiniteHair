#pragma once
#include <string>

namespace logfile {

void open(const std::string& path);
void write(const char* fmt, ...);

} // namespace logfile
