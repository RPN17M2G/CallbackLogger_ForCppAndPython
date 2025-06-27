#include "CallbackLogger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

enum class MyComponent { NETWORK, DATABASE, UI };
enum class MyOtherComponent { SYSTEM, AUTHENTICATION };

int main() {
    CallbackLogger logger(1); // Add one worker thread

    // Register a function callback for Info severity
    logger.register_function_callback(
        [](const LogEntry& entry) {
            std::cout << "[INFO] " << entry.component.to_string() << ": "
                      << entry.message << " (" << entry.file << ":" << entry.line << ")\n";
        },
        Severity::Info
    );

    // Register a callback for warnings on UI component and fatal on Database component
    logger.register_function_callback(
        [](const LogEntry& entry) {
            std::cout << "[WARNING or FATAL] " << entry.message << " in " << entry.component.to_string() << std::endl;
        },
        std::unordered_map<MyComponent, Severity>{
            {MyComponent::UI, Severity::Warning},
            {MyComponent::DATABASE, Severity::Fatal}
        }
    );

    // Register a callback for errors on AUTHENTICATION component
    logger.register_function_callback(
        [](const LogEntry& entry) {
            std::cerr << "[ERROR] " << entry.message << " (" << entry.file << ":" << entry.line << ")\n";
        },
        std::unordered_map<MyOtherComponent, Severity>{
            {MyOtherComponent::AUTHENTICATION, Severity::Error}
        }
    );

    // Register a file callback for all logs
    logger.register_file_callback("all_logs_cpp.log", Severity::Debug);

    // Register a file callback for warnings on DATABASE
    logger.register_file_callback("db_warnings_cpp.log",
        std::unordered_map<MyComponent, Severity>{
            {MyComponent::DATABASE, Severity::Warning}
        }
    );

    // Log messages with various severities and components
    logger.log(Severity::Info, MyComponent::NETWORK, "Network initialized", __FILE__, __LINE__);
    logger.log(Severity::Warning, MyComponent::UI, "UI lag detected", __FILE__, __LINE__);
    logger.log(Severity::Error, MyComponent::DATABASE, "Database connection failed", __FILE__, __LINE__);

    // Use cpp macro for automatic adding of file and line
    LOG(logger, Severity::Debug, MyComponent::UI, "UI redraw event");
    LOG(logger, Severity::Info, MyComponent::DATABASE, "Database query executed");
    LOG(logger, Severity::Info, MyComponent::NETWORK, "Network packet sent");

    std::cout << "\nCheck 'all_logs_cpp.log' and 'db_warnings_cpp.log' for file output." << std::endl;

    // Give time for async logging
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
