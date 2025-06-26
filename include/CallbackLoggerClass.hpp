#pragma once
#include <unordered_map>
#include <set>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <memory>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "Utils/SeverityUtils.hpp"
#include "Models/CallbackFilters.hpp"
#include "Utils/LoggerInternalCallbacks.hpp"
#include "Models/LogEntry.hpp"
#include "Models/ComponentEnumEntry.hpp"
#include "Models/Severity.hpp"

using Task = std::function<void()>;

class CallbackLogger
{
public:
    /**
     * @brief Constructs a CallbackLogger with a specified number of worker threads.
     *
     * @param thread_count Number of threads in the thread pool. If 0, logger is single-threaded.
     */
    explicit CallbackLogger(size_t thread_count = DEFAULT_THREAD_COUNT);

    /**
     * @brief Destructor. Stops all worker threads and cleans up resources.
     */
    ~CallbackLogger();

    /**
     * @brief Stops all worker threads and cleans up resources.
     */
    void shutdown();

    /**
     * @brief Registers a function callback with a full component and severity filter.
     *
     * @param callback The callback function to register.
     * @param filter Map of components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback,
                                   const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>& filter = {});

    /**
     * @brief Registers a function callback with a components filter.
     *
     * @param callback The callback function to register.
     * @param component_filter Set of components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback,
                                   const std::set<ComponentEnumEntry>& component_filter);

    /**
     * @brief Registers a function callback for all components with a minimum severity.
     *
     * @param callback The callback function to register.
     * @param min_severity Minimum severity for all components.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback,
                                   Severity min_severity);

    /**
     * @brief Registers a function callback for a specific component.
     *
     * @param callback The callback function to register.
     * @param component The component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback, ComponentEnumEntry component);

    /**
     * @brief Registers a file callback for a specific component.
     *
     * @param filename The file to write logs to.
     * @param component The component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename, ComponentEnumEntry component);

    /**
     * @brief Registers a file callback with a full component and severity filter.
     *
     * @param filename The file to write logs to.
     * @param filter Map of components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename,
                               const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>& filter = {});

    /**
     * @brief Registers a file callback with a components filter.
     *
     * @param filename The file to write logs to.
     * @param component_filter Set of components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename,
                               const std::set<ComponentEnumEntry>& component_filter);

    /**
     * @brief Registers a file callback for all components with a minimum severity.
     *
     * @param filename The file to write logs to.
     * @param min_severity Minimum severity for all components.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename,
                               Severity min_severity);

    /**
     * @brief Unregisters a function callback.
     *
     * @param handle The handle of the callback to unregister.
     */
    void unregister_function_callback(uint32_t handle);

    /**
     * @brief Unregisters a file callback.
     *
     * @param handle The handle of the file callback to unregister.
     */
    void unregister_file_callback(uint32_t handle);

    /**
     * @brief Registers a function callback for a specific enum component.
     *
     * @tparam EnumT Enum type.
     * @param callback The callback function to register.
     * @param component The enum component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback, EnumT component)
    {
        return register_function_callback(callback, make_component_entry(component));
    }

    /**
     * @brief Registers a file callback for a specific enum component.
     *
     * @tparam EnumT Enum type.
     * @param filename The file to write logs to.
     * @param component The enum component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_file_callback(const std::string& filename, EnumT component)
    {
        return register_file_callback(filename, make_component_entry(component));
    }

    /**
     * @brief Logs a message asynchronously for a specific enum component.
     *
     * @tparam EnumT Enum type.
     * @param severity The severity level of the log.
     * @param component The enum component generating the log.
     * @param message The log message.
     * @param file The source file where the log was generated.
     * @param line The line number in the source file.
     */
    template <typename EnumT>
    void log(Severity severity, EnumT component, const std::string& message,
             const std::string& file, uint32_t line)
    {
        log(severity, make_component_entry(component), message, file, line);
    }

    /**
     * @brief Registers a function callback for a set of enum components.
     *
     * @tparam EnumT Enum type.
     * @param callback The callback function to register.
     * @param component_filter Set of enum components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback, const std::set<EnumT>& component_filter)
    {
        std::set<ComponentEnumEntry> entries;
        for (const auto& c : component_filter) entries.insert(make_component_entry(c));
        return register_function_callback(callback, entries);
    }

    /**
     * @brief Registers a file callback for a set of enum components.
     *
     * @tparam EnumT Enum type.
     * @param filename The file to write logs to.
     * @param component_filter Set of enum components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_file_callback(const std::string& filename, const std::set<EnumT>& component_filter)
    {
        std::set<ComponentEnumEntry> entries;
        for (const auto& c : component_filter) entries.insert(make_component_entry(c));
        return register_file_callback(filename, entries);
    }

    /**
     * @brief Registers a function callback with a map of enum components to minimum severities.
     *
     * @tparam EnumT Enum type.
     * @param callback The callback function to register.
     * @param filter Map of enum components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_function_callback(const std::function<void(const LogEntry&)>& callback,
        const std::unordered_map<EnumT, Severity>& filter)
    {
        std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> entries;
        for (const auto& kv : filter) entries.emplace(make_component_entry(kv.first), kv.second);
        return register_function_callback(callback, entries);
    }

    /**
     * @brief Registers a file callback with a map of enum components to minimum severities.
     *
     * @tparam EnumT Enum type.
     * @param filename The file to write logs to.
     * @param filter Map of enum components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    template <typename EnumT>
    uint32_t register_file_callback(const std::string& filename,
        const std::unordered_map<EnumT, Severity>& filter)
    {
        std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> entries;
        for (const auto& kv : filter) entries.emplace(make_component_entry(kv.first), kv.second);
        return register_file_callback(filename, entries);
    }

    /**
     * @brief Logs a message asynchronously.
     *
     * @param severity The severity level of the log.
     * @param component The component generating the log.
     * @param message The log message.
     * @param file The source file where the log was generated.
     * @param line The line number in the source file.
     */
    void log(Severity severity, const ComponentEnumEntry& component, const std::string& message,
             const std::string& file, uint32_t line);

private:
    /**
     * @brief Checks if a log entry matches a callback's filter.
     *
     * @param filter The filter to check against.
     * @param severity The severity of the log entry.
     * @param component The component of the log entry.
     * @return True if the entry matches the filter, false otherwise.
     */
    bool _is_matching_callback_filter(
        const std::variant<std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>, Severity>& filter,
        const Severity severity, const ComponentEnumEntry component) const;

    /**
     * @brief Gets the current timestamp as a string.
     *
     * @return The current timestamp.
     */
    std::string _get_current_timestamp() const;

    /**
     * @brief Worker thread function that processes log tasks from the queue.
     */
    void _worker_thread();

    /**
     * @brief Asynchronous log implementation (enqueues tasks).
     *
     * @param entry The log entry to process.
     */
    void _async_log(const LogEntry& entry);

    /**
     * @brief Single-threaded log implementation (directly executes callbacks).
     *
     * @param entry The log entry to process.
     */
    void _single_threaded_log(const LogEntry& entry);

    std::unordered_map<uint32_t, FunctionCallbackFilterPtr> m_function_callbacks;
    std::unordered_map<uint32_t, FileCallbackFilterPtr> m_file_callbacks;
    std::atomic<uint32_t> m_next_callback_handle{1};
    mutable std::mutex m_register_mutex;

    bool m_single_threaded{false};

    std::queue<std::function<void()>> m_task_queue;
    std::vector<std::thread> m_workers;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    std::atomic<bool> m_stopping{false};

    constexpr static size_t DEFAULT_THREAD_COUNT = 1;
};