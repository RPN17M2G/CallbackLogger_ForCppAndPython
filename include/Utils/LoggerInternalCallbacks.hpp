#pragma once

#include <string>
#include <fstream>

#include "Models/Severity.hpp"
#include "Models/LogEntry.hpp"
#include "Utils/SeverityUtils.hpp"

/**
 * @brief Writes a log entry to a file, opening the file for appending only for this operation.
 *
 * @param entry The log entry to write.
 * @param file_path The path to the file.
 */
void file_log_callback(const LogEntry& entry, const std::string& file_path);
