#pragma once

#include <cstdint>

#include "CallbackLoggerClass.hpp"

// RAII class to automatically unregister a file callback
class AutoCloseFileCallback
{
public:
    AutoCloseFileCallback(uint32_t handle, CallbackLoggerPtr logger)
        : m_handle(handle), m_logger(logger) {}

    virtual ~AutoCloseFileCallback() { m_logger->unregister_file_callback(m_handle); }

    AutoCloseFileCallback(AutoCloseFileCallback& other) = delete;
    AutoCloseFileCallback& operator=(const AutoCloseFileCallback& other) = delete;

private:
    uint32_t m_handle;
    CallbackLoggerPtr m_logger;
};

