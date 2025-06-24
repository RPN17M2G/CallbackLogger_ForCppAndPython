#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "ComponentEnumEntry.hpp"
#include "Severity.hpp"
#include "Models/LogEntry.hpp"

using LogCallback = std::function<void(const LogEntry&)>;

/**
 * @brief Holds file path and filter for file logging.
 */
struct FileCallBackFilter
{
    std::string file_path;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> component_min_severity;
};
using FileCallbackFilterPtr = std::shared_ptr<FileCallBackFilter>;

/**
 * @brief Holds a function callback and its filter.
 */
struct FunctionCallbackFilter
{
    const LogCallback callback_function;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> component_min_severity;
};
using FunctionCallbackFilterPtr = std::shared_ptr<FunctionCallbackFilter>;
