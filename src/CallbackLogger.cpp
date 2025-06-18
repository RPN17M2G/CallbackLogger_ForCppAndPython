#include "CallbackLogger.hpp"

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

std::string to_string(const Component component)
{
    constexpr std::array<const char*, 3> ComponentToStringMap = {
        "System", "Module", "PacketProcessor"
    };

    const uint32_t index = static_cast<uint32_t>(component);
    if (index < ComponentToStringMap.size())
        return ComponentToStringMap[index];
    return "UnknownComponent";
}

CallbackLogger::CallbackLogger(size_t thread_count)
{
    if (thread_count == 0) thread_count = DEFAULT_THREAD_COUNT;
    m_stopping = false;
    for (size_t worker_count = 0; worker_count < thread_count; ++worker_count)
        m_workers.emplace_back(&CallbackLogger::worker_thread, this);
}

CallbackLogger::~CallbackLogger()
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
    const std::unordered_map<Component, Severity>& filter)
{
    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_function_callbacks[handle] = std::make_shared<FunctionCallbackFilter>(FunctionCallbackFilter{callback, filter});
    return handle;
}

uint32_t CallbackLogger::register_function_callback(
    const LogCallback& callback,
    const std::set<Component>& component_filter)
{
    std::unordered_map<Component, Severity> filter;
    for (const auto& component : component_filter)
        filter[component] = Severity::Uninitialized;
    return register_function_callback(callback, filter);
}

uint32_t CallbackLogger::register_function_callback(
    const LogCallback& callback,
    const Severity min_severity)
{
    std::unordered_map<Component, Severity> filter;
    for (uint32_t component = 0; component < static_cast<uint32_t>(Component::COMPONENT_COUNT); ++component)
        filter[static_cast<Component>(component)] = min_severity;
    return register_function_callback(callback, filter);
}

uint32_t CallbackLogger::register_function_callback(const LogCallback& callback, const Component component)
{
    std::set<Component> component_set{component};
    return register_function_callback(callback, component_set);
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const std::unordered_map<Component, Severity>& filter)
{
    std::lock_guard<std::mutex> lock(m_register_mutex);
    uint32_t handle = m_next_callback_handle++;
    m_file_callbacks[handle] = std::make_shared<FileCallbackFilter>(FileCallbackFilter{std::ofstream(filename, std::ios::app), filter});
    return handle;
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const std::set<Component>& component_filter)
{
    std::unordered_map<Component, Severity> filter;
    for (const auto& component : component_filter)
        filter[component] = Severity::Uninitialized;
    return register_file_callback(filename, filter);
}

uint32_t CallbackLogger::register_file_callback(
    const std::string& filename,
    const Severity min_severity)
{
    std::unordered_map<Component, Severity> filter;
    for (uint32_t component = 0; component < static_cast<uint32_t>(Component::COMPONENT_COUNT); ++component)
        filter[static_cast<Component>(component)] = min_severity;
    return register_file_callback(filename, filter);
}

uint32_t CallbackLogger::register_file_callback(const std::string& filename, const Component component)
{
    std::set<Component> component_set{component};
    return register_file_callback(filename, component_set);
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
    if (m_function_callbacks.find(handle) == m_function_callbacks.end())
    {
        throw std::runtime_error("Callback handle not found: " + std::to_string(handle));
    }
    m_file_callbacks.erase(handle);
}

void CallbackLogger::log(const Severity severity, const Component component, const std::string& message,
                         const std::string& file, const uint32_t line)
{
    const std::string timestamp = get_current_timestamp();
    LogEntry entry{severity, component, message, file, line, timestamp};

    // Copy callbacks for enabling registration while logging
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
            if (is_matching_callback_filter(callback->component_min_severity, severity, component) && (callback->file_stream.is_open()))
            {
                m_task_queue.emplace([entry, callback]() mutable {
                    constexpr const char* ERROR_PREFIX = "[!] ";
                    constexpr const char* INFO_PREFIX = "[*] ";

                    callback->file_stream << ((entry.severity >= Severity::Warning) ? ERROR_PREFIX : INFO_PREFIX)
                        << "[" << entry.timestamp << "] [" << to_string(entry.severity) << "] "
                        << to_string(entry.component) << " (" << entry.file << ":" << entry.line << "): "
                        << entry.message << std::endl;
                });
            }
        }

        for (const FunctionCallbackFilterPtr& callback : function_callbacks)
        {
            if (is_matching_callback_filter(callback->component_min_severity, severity, component))
            {
                m_task_queue.emplace([callback_function = callback->callback_function, entry]() {
                    callback_function(entry);
                });
            }
        }
    }
    m_queue_condition.notify_all();
}

void CallbackLogger::worker_thread()
{
    while (true) {
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

bool CallbackLogger::is_matching_callback_filter(const std::unordered_map<Component, Severity>& filter,
                                  const Severity severity, const Component component) const
{
    if (filter.empty())
        return true;
    auto it = filter.find(component);
    if (it != filter.end())
        return severity >= it->second;
    return false;
}

std::string CallbackLogger::get_current_timestamp() const
{
    constexpr size_t ms_width = 3;
    constexpr size_t to_milliseconds = 1000;

    const auto now = std::chrono::system_clock::now();
    const std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % to_milliseconds;

    std::ostringstream string_stream;
    string_stream << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(ms_width) << ms.count();
    return string_stream.str();
}
