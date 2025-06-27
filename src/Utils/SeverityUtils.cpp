#include "Utils/SeverityUtils.hpp"

std::string to_string(const Severity severity)
{
    constexpr std::array<const char*, 5> SeverityToStringMap = {
        "Debug", "Info", "Warning", "Error", "Fatal"
    };

    const uint32_t index = static_cast<uint32_t>(severity);
    if (index < SeverityToStringMap.size())
        return SeverityToStringMap[index];
    return "UnknownSeverity";
}
