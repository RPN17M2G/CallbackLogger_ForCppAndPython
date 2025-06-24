#include "Utils/LoggerInternalCallbacks.hpp"

void file_log_callback(const LogEntry& entry, const std::string& file_path)
{
    std::ofstream file_stream(file_path, std::ios::app);
    if (!file_stream.is_open()) return;
    constexpr const char* ERROR_PREFIX = "[!] ";
    constexpr const char* INFO_PREFIX = "[*] ";
    file_stream << ((entry.severity >= Severity::Warning) ? ERROR_PREFIX : INFO_PREFIX)
        << "[" << entry.timestamp << "] [" << to_string(entry.severity) << "] "
        << entry.component.to_string() << " (" << entry.file << ":" << entry.line << "): "
        << entry.message << std::endl;
}
