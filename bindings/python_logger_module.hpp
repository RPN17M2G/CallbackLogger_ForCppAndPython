#pragma once
#include <pybind11/pybind11.h>

namespace py = pybind11;

/**
 * @brief Initializes the pycallbacklogger Python module.
 *
 * @param m The pybind11 module.
 */
void init_python_logger_module(py::module_& m);
