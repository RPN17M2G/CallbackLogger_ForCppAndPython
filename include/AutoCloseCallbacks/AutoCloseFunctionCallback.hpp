#pragma once

#include <cstdint>

#include "CallbackLoggerClass.hpp"

// RAII class to automatically unregister a function callback
class AutoCloseFunctionCallback
{
public:
    AutoCloseFunctionCallback(uint32_t handle, CallbackLoggerPtr logger)
        : m_handle(handle), m_logger(logger) {}

    virtual ~AutoCloseFunctionCallback() { m_logger->unregister_function_callback(m_handle); }

    AutoCloseFunctionCallback(AutoCloseFunctionCallback& other) = delete;
    AutoCloseFunctionCallback& operator=(const AutoCloseFunctionCallback& other) = delete;

private:
    uint32_t m_handle;
    CallbackLoggerPtr m_logger;
};

