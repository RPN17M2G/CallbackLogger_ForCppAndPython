#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <set>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <array>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <iostream>

enum class Severity
{
    Uninitialized = -1,
    Debug = 0,
    Info,
    Warning,
    Error,
    Fatal,
    SEVERITY_COUNT
};

enum class Component
{
    S,
    M,
    P,
    COMPONENT_COUNT
};

/**
 * @brief Converts a Severity enum to its string representation.
 * @param severity The severity level to convert.
 * @return String representation of the severity.
 */
std::string to_string(Severity severity);

/**
 * @brief Converts a Component enum to its string representation.
 * @param component The component to convert.
 * @return String representation of the component.
 */
std::string to_string(Component component);

struct LogEntry
{
    Severity severity;
    Component component;
    std::string message;
    std::string file;
    uint32_t line;
    std::string timestamp;
};

using Task = std::function<void()>;
using LogCallback = std::function<void(const LogEntry&)>;

struct FunctionCallbackFilter
{
    LogCallback callback_function;
    std::unordered_map<Component, Severity> component_min_severity;
};
using FunctionCallbackFilterPtr = std::shared_ptr<FunctionCallbackFilter>;

struct FileCallbackFilter
{
    std::ofstream file_stream;
    std::unordered_map<Component, Severity> component_min_severity;
};
using FileCallbackFilterPtr = std::shared_ptr<FileCallbackFilter>;

class CallbackLogger
{
public:
    /**
     * @param thread_count Number of threads in the thread pool - defaults to the number of hardware threads available.
     */
    explicit CallbackLogger(size_t thread_count = DEFAULT_THREAD_COUNT);

    /**
     * @brief Destructor. Stops all worker threads and cleans up resources.
     */
    ~CallbackLogger();

    /**
     * @brief Registers a function callback with a full component and severity filter.
     *
     * @param callback The callback function to register.
     * @param filter Map of components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const LogCallback& callback,
                                   const std::unordered_map<Component, Severity>& filter = {});

    /**
     * @brief Registers a function callback with a components filter.
     *
     * @param callback The callback function to register.
     * @param component_filter Set of components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const LogCallback& callback,
                                   const std::set<Component>& component_filter);

    /**
     * @brief Registers a function callback for all components with a minimum severity.
     *
     * @param callback The callback function to register.
     * @param min_severity Minimum severity for all components.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const LogCallback& callback,
                                   Severity min_severity);

    /**
     * @brief Registers a function callback for a specific component.
     *
     * @param callback The callback function to register.
     * @param component The component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_function_callback(const LogCallback& callback, Component component);

    /**
     * @brief Registers a file callback for a specific component.
     *
     * @param filename The file to write logs to.
     * @param component The component to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename, Component component);

    /**
     * @brief Registers a file callback with a full component and severity filter.
     *
     * @param filename The file to write logs to.
     * @param filter Map of components to minimum severities for filtering.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename,
                               const std::unordered_map<Component, Severity>& filter = {});

    /**
     * @brief Registers a file callback with a components filter.
     *
     * @param filename The file to write logs to.
     * @param component_filter Set of components to filter.
     * @return Handle to the callback, which can be used to unregister it.
     */
    uint32_t register_file_callback(const std::string& filename,
                               const std::set<Component>& component_filter);

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
     * @brief Logs a message asynchronously.
     *
     * @param severity The severity level of the log.
     * @param component The component generating the log.
     * @param message The log message.
     * @param file The source file where the log was generated.
     * @param line The line number in the source file.
     */
    void log(Severity severity, Component component, const std::string& message,
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
    bool is_matching_callback_filter(const std::unordered_map<Component, Severity>& filter,
                       Severity severity, Component component) const;

    /**
     * @brief Gets the current timestamp as a string.
     * @return The current timestamp.
     */
    std::string get_current_timestamp() const;

    /**
     * @brief Worker thread function that processes log tasks from the queue.
     */
    void worker_thread();

    std::unordered_map<uint32_t, FunctionCallbackFilterPtr> m_function_callbacks;
    std::unordered_map<uint32_t, FileCallbackFilterPtr> m_file_callbacks;
    std::atomic<uint32_t> m_next_callback_handle{1};
    mutable std::mutex m_register_mutex;

    std::queue<Task> m_task_queue;
    std::vector<std::thread> m_workers;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    std::atomic<bool> m_stopping{false};

    constexpr static size_t DEFAULT_THREAD_COUNT = 1;
};

#define LOG(logger, severity, component, message) \
    (logger).log((severity), (component), (message), __FILE__, __LINE__)
