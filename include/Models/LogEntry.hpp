#pragma once
#include <string>
#include <cstdint>
#include "Severity.hpp"
#include "ComponentEnumEntry.hpp"

struct LogEntry
{
    Severity severity;
    ComponentEnumEntry component;
    std::string message;
    std::string file;
    uint32_t line;
    std::string timestamp;
};
