#pragma once
#include <pybind11/pybind11.h>
#include "CallbackLoggerClass.hpp"

namespace py = pybind11;

/**
 * @brief Registers the PyCallbackLogger class and its methods with the module.
 *
 * @param m The pybind11 module.
 */
void register_python_logger_utils(py::module_& m);
