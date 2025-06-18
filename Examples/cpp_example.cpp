#include <cmath>

#include "CallbackLogger.hpp"
#include <iostream>

int main() {
    CallbackLogger logger;

    // Register a function callback (prints to console)
    uint32_t handle = logger.register_function_callback(
        [](const LogEntry& entry) {
            std::cout << "[" << to_string(entry.severity) << "] "
                      << to_string(entry.component) << ": "
                      << entry.message << std::endl;
        },
        Severity::Debug // minimum severity
    );

    // Register a file callback (logs to "log.txt")
    logger.register_file_callback("C:/Users/omerg/OneDrive/Documents/Army/Remote/log_infrastructure/log.txt", Severity::Info);

    // Log a message
    logger.log(Severity::Info, Component::S, "Hello from C++!", __FILE__, __LINE__);
    logger.log(Severity::Info, Component::S, "Hello 12 C++!", __FILE__, __LINE__);

    logger.unregister_function_callback(567);
    logger.log(Severity::Info, Component::S, "Hello 133 C++!", __FILE__, __LINE__);
    logger.log(Severity::Info, Component::S, "Hello 444 C++!", __FILE__, __LINE__);

    return 0;
}
