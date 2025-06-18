#include "LoggingFunctionsExamples.hpp"

void startup_function_that_logs(CallbackLogger& logger)
{
    LOG(logger, Severity::Info, Component::S, "System startup");
}

void function_that_logs(CallbackLogger& logger)
{
    LOG(logger, Severity::Info, Component::S, "Function called with detail: ");
    LOG(logger, Severity::Warning, Component::S, "Warning: " );
    LOG(logger, Severity::Error, Component::S, "Error encountered with detail: " );
    LOG(logger, Severity::Debug, Component::S, "Debug info for: ");
}