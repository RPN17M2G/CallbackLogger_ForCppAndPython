#include "CallbackLoggerClass.hpp"

CallbackLogger::CallbackLogger(size_t thread_count)
{
    if (thread_count == 0)
    {
        m_single_threaded = true;
    }
    else
    {
        m_single_threaded = false;
        for (size_t worker_count = 0; worker_count < thread_count; ++worker_count)
            m_workers.emplace_back(&CallbackLogger::_worker_thread, this);
    }
    m_stopping = false;

}

CallbackLogger::~CallbackLogger()
{
    shutdown();
}

void CallbackLogger::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_stopping = true;
        m_queue_condition.notify_all();
    }
    for (std::thread& worker : m_workers)
        if (worker.joinable()) worker.join();
}

uint32_t CallbackLogger::register_function_callback(
    const LogCallback& callback,
    const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>& filter)
{
    if (!callback)
    {
        throw std::invalid_argument("Function callback cannot be null");
    }
    for (const auto& pair : filter)
    {
        if (pair.second < Severity::Debug || pair.second > Severity::Fatal)
        {
            throw std::invalid_argument("Invalid severity in filter map for function callback registration");
        }
    }
    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_function_callbacks[handle] = std::make_shared<FunctionCallbackFilter>(
        FunctionCallbackFilter{callback, filter});
    return handle;
}

uint32_t CallbackLogger::register_function_callback(
    const LogCallback& callback,
    const std::set<ComponentEnumEntry>& component_filter)
{
    if (!callback)
    {
        throw std::invalid_argument("Function callback cannot be null");
    }
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    for (const ComponentEnumEntry& component : component_filter)
        filter[component] = Severity::Debug;
    return register_function_callback(callback, filter);
}

uint32_t CallbackLogger::register_function_callback(
    const LogCallback& callback,
    const Severity min_severity)
{
    if (!callback)
    {
        throw std::invalid_argument("Function callback cannot be null");
    }
    if (min_severity < Severity::Debug || min_severity > Severity::Fatal)
    {
        throw std::invalid_argument("Invalid severity for function callback registration");
    }
    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_function_callbacks[handle] = std::make_shared<FunctionCallbackFilter>(
        FunctionCallbackFilter{callback, min_severity});
    return handle;
}

uint32_t CallbackLogger::register_function_callback(const LogCallback& callback, const ComponentEnumEntry component)
{
    if (!callback)
    {
        throw std::invalid_argument("Function callback cannot be null");
    }
    return register_function_callback(callback, std::set<ComponentEnumEntry>{component});
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>& filter)
{
    std::ofstream file_stream(filename, std::ios::app);
    if (!file_stream)
    {
        throw std::invalid_argument("Invalid log file path: " + filename);
    }

    for (const std::pair<ComponentEnumEntry, Severity>& pair : filter)
    {
        if (pair.second < Severity::Debug || pair.second > Severity::Fatal)
        {
            throw std::invalid_argument("Invalid severity in filter map for file callback registration");
        }
    }

    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_file_callbacks[handle] = std::make_shared<FileCallBackFilter>(
        FileCallBackFilter{filename, filter});
    return handle;
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const std::set<ComponentEnumEntry>& component_filter)
{
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    for (const ComponentEnumEntry& component : component_filter)
        filter[component] = Severity::Debug;
    return register_file_callback(filename, filter);
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const Severity min_severity)
{
    if (min_severity < Severity::Debug || min_severity > Severity::Fatal)
    {
        throw std::invalid_argument("Invalid severity for file callback registration");
    }
    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_file_callbacks[handle] = std::make_shared<FileCallBackFilter>(
        FileCallBackFilter{filename, min_severity});
    return handle;
}

uint32_t CallbackLogger::register_file_callback(const std::string& filename, const ComponentEnumEntry component)
{
    if (filename.empty())
    {
        throw std::invalid_argument("Filename for file callback cannot be empty");
    }
    return register_file_callback(filename, std::set<ComponentEnumEntry>{component});
}

void CallbackLogger::unregister_function_callback(uint32_t handle)
{
    std::lock_guard<std::mutex> lock(m_register_mutex);
    if (m_function_callbacks.find(handle) == m_function_callbacks.end())
    {
        throw std::runtime_error("Callback handle not found: " + std::to_string(handle));
    }
    m_function_callbacks.erase(handle);
}

void CallbackLogger::unregister_file_callback(uint32_t handle)
{
    std::lock_guard<std::mutex> lock(m_register_mutex);
    if (m_file_callbacks.find(handle) == m_file_callbacks.end())
    {
        throw std::runtime_error("Callback handle not found: " + std::to_string(handle));
    }
    m_file_callbacks.erase(handle);
}

void CallbackLogger::log(const Severity severity, const ComponentEnumEntry& component, const std::string& message,
                         const std::string& file, const uint32_t line)
{
    if (message.empty())
    {
        throw std::runtime_error("Cannot log an empty message");
    }
    if (file.empty())
    {
        throw std::runtime_error("Cannot log without a file name");
    }
    if (line <= 0)
    {
        throw std::runtime_error("Cannot log without a line number");
    }
    if (severity < Severity::Debug || severity > Severity::Fatal)
    {
        throw std::runtime_error("Invalid severity level: " + std::to_string(static_cast<int>(severity)));
    }

    const LogEntry entry{severity, component, message, file, line, get_current_timestamp()};
    if (m_single_threaded)
    {
        _single_threaded_log(entry);
    } else {
        _async_log(entry);
    }
}

bool CallbackLogger::_is_matching_callback_filter(
    const std::variant<std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>, Severity>& filter,
    const Severity severity, const ComponentEnumEntry component) const
{
    if (std::holds_alternative<Severity>(filter))
    {
        return severity >= std::get<Severity>(filter);
    }
    const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>& map = std::get<std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>>(filter);
    if (map.empty())
        return true;

    auto component_iterator = map.find(component);
    if (component_iterator != map.end())
        return severity >= component_iterator->second;
    return false;
}

void CallbackLogger::_async_log(const LogEntry& entry)
{
    std::vector<FunctionCallbackFilterPtr> function_callbacks;
    std::vector<FileCallbackFilterPtr> file_callbacks;
    {
        std::lock_guard<std::mutex> lock(m_register_mutex);
        for (const std::pair<uint32_t, FunctionCallbackFilterPtr>& callback_pair : m_function_callbacks)
            function_callbacks.push_back(callback_pair.second);
        for (const std::pair<uint32_t, FileCallbackFilterPtr>& callback_pair : m_file_callbacks)
            file_callbacks.push_back(callback_pair.second);
    }

    // Enqueue a task for each callback
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);

        for (FileCallbackFilterPtr& callback : file_callbacks)
        {
            if (_is_matching_callback_filter(callback->filter, entry.severity, entry.component))
            {
                m_task_queue.emplace([entry, callback]() mutable {
                    file_log_callback(entry, callback->file_path);
                });
            }
        }

        for (const FunctionCallbackFilterPtr& callback : function_callbacks)
        {
            if (_is_matching_callback_filter(callback->filter, entry.severity, entry.component))
            {
                m_task_queue.emplace([callback, entry]()
{
                    callback->callback_function(entry);
                });
            }
        }
    }
    m_queue_condition.notify_all();
}

void CallbackLogger::_single_threaded_log(const LogEntry& entry)
{
    for (const std::pair<const uint32_t, FileCallbackFilterPtr>& callback : m_file_callbacks)
    {
        if (_is_matching_callback_filter(callback.second->filter, entry.severity, entry.component))
        {
            try
            {
                 file_log_callback(entry, callback.second->file_path);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[!] Exception while handling file callback: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "[!] Unknown exception while handling file callback." << std::endl;
            }
        }
    }

    for (const std::pair<const uint32_t, FunctionCallbackFilterPtr>& callback : m_function_callbacks)
    {
        if (_is_matching_callback_filter(callback.second->filter, entry.severity, entry.component))
        {
            try
            {
                callback.second->callback_function(entry);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[!] Exception while handling function callback: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "[!] Unknown exception while handling function callback." << std::endl;
            }
        }
    }
}

void CallbackLogger::_worker_thread()
{
    while (true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this] { return m_stopping || !m_task_queue.empty(); });
            if (m_stopping && m_task_queue.empty())
                return;
            if (!m_task_queue.empty())
            {
                task = std::move(m_task_queue.front());
                m_task_queue.pop();
            }
            else
            {
                continue;
            }
        }
        if (task)
        {
            try
            {
                task();
            }
            catch (const std::exception& e)
            {
                std::cerr << "[!] Exception in worker thread: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "[!] Unknown exception in worker thread." << std::endl;
            }
        }
    }
}
