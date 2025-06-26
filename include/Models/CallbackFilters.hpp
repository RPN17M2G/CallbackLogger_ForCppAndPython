#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>

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
    std::variant<
        std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>,
        Severity
    > filter;
};
using FileCallbackFilterPtr = std::shared_ptr<FileCallBackFilter>;

/**
 * @brief Holds a function callback and its filter.
 */
struct FunctionCallbackFilter
{
    const LogCallback callback_function;
    std::variant<
        std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>,
        Severity
    > filter;
};
using FunctionCallbackFilterPtr = std::shared_ptr<FunctionCallbackFilter>;

