#pragma once
#include <pybind11/pybind11.h>
#include <string>
#include "Models/ComponentEnumEntry.hpp"
#include "Models/LogEntry.hpp"
#include "Models/Severity.hpp"

namespace py = pybind11;

/**
 * @brief Converts a Python enum object to a ComponentEnumEntry.
 *
 * @param enum_object The Python enum object.
 * @return The corresponding ComponentEnumEntry.
 */
ComponentEnumEntry py_enum_to_entry(const py::object& enum_object);

/**
 * @brief Registers Python types (Severity, LogEntry, ComponentEnumEntry) with the module.
 *
 * @param m The pybind11 module.
 */
void register_python_logger_types(py::module_& m);
