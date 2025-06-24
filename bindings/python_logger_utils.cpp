#include "python_logger_utils.hpp"
#include "python_logger_types.hpp"
#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class PyCallbackLogger : public CallbackLogger
{
public:
    PyCallbackLogger() : CallbackLogger(0) // Always single-threaded for Python because of GIL(Global Interpreter Lock)
    {
        py::object self = py::cast(this);
    }
};

void register_python_logger_utils(py::module_& m)
{
    py::class_<CallbackLogger>(m, "CallbackLoggerBase")
        .def(py::init<>())
        .def("shutdown", &CallbackLogger::shutdown)
        .def("unregister_function_callback", &CallbackLogger::unregister_function_callback, py::arg("handle"))
        .def("unregister_file_callback", &CallbackLogger::unregister_file_callback, py::arg("handle"));

    py::class_<PyCallbackLogger, CallbackLogger>(m, "CallbackLogger")
        .def(py::init<>())
        .def("shutdown", &CallbackLogger::shutdown)
        .def("register_function_callback",
            [](CallbackLogger& logger, py::function py_callback, py::object filter)
            {
                LogCallback safe_callback = [py_callback](const LogEntry& entry)
                {
                    if (!Py_IsInitialized()) return;
                    try
                    {
                        py::gil_scoped_acquire gil;
                        py_callback(entry);
                    }
                    catch (const std::exception &e)
                    {
                        py::print("[!] Exception in Python callback:", e.what());
                    }
                    catch (...)
                    {
                        py::print("[!] Unknown exception in Python callback.");
                    }
                };

                if (py::isinstance<py::dict>(filter))
                {
                    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> native_filter;
                    for (std::pair<pybind11::handle, pybind11::handle> item : filter.cast<py::dict>())
                    {
                        ComponentEnumEntry entry = py_enum_to_entry(static_cast<const py::object &>(item.first));
                        native_filter[entry] = item.second.cast<Severity>();
                    }
                    return logger.register_function_callback(safe_callback, native_filter);
                }
                else if (py::isinstance<py::set>(filter))
                {
                    std::set<ComponentEnumEntry> native_set;
                    for (pybind11::handle item : filter.cast<py::set>())
                    {
                        native_set.insert(py_enum_to_entry(py::reinterpret_borrow<py::object>(item)));
                    }
                    return logger.register_function_callback(safe_callback, native_set);
                }
                else if (py::hasattr(filter, "value"))
                {
                    ComponentEnumEntry entry = py_enum_to_entry(filter);
                    return logger.register_function_callback(safe_callback, entry);
                }
                else if (py::isinstance<Severity>(filter))
                {
                    return logger.register_function_callback(safe_callback, filter.cast<Severity>());
                }
                else if (filter.is_none())
                {
                    return logger.register_function_callback(safe_callback, Severity::Uninitialized);
                }
                else
                {
                    throw std::runtime_error("Unsupported filter type for register_function_callback");
                }
            }, py::arg("callback"), py::arg("filter") = py::none())
        .def("register_file_callback",
            [](CallbackLogger& logger, const std::string& filename, py::object filter)
            {
                if (py::isinstance<py::dict>(filter))
                {
                    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> native_filter;
                    for (std::pair<pybind11::handle, pybind11::handle> item : filter.cast<py::dict>())
                    {
                        ComponentEnumEntry entry = py_enum_to_entry(static_cast<const py::object &>(item.first));
                        native_filter[entry] = item.second.cast<Severity>();
                    }
                    return logger.register_file_callback(filename, native_filter);
                }
                else if (py::isinstance<py::set>(filter))
                {
                    std::set<ComponentEnumEntry> native_set;
                    for (pybind11::handle item : filter.cast<py::set>())
                    {
                        native_set.insert(py_enum_to_entry(static_cast<const py::object &>(item)));
                    }
                    return logger.register_file_callback(filename, native_set);
                }
                else if (py::hasattr(filter, "value"))
                {
                    ComponentEnumEntry entry = py_enum_to_entry(filter);
                    return logger.register_file_callback(filename, entry);
                }
                else if (py::isinstance<Severity>(filter))
                {
                    return logger.register_file_callback(filename, filter.cast<Severity>());
                }
                else if (filter.is_none())
                {
                    return logger.register_file_callback(filename, Severity::Uninitialized);
                }
                else
                {
                    throw std::runtime_error("Unsupported filter type for register_file_callback");
                }
            }, py::arg("filename"), py::arg("filter") = py::none())
        .def("log",
            [](CallbackLogger& logger, Severity severity, py::object component, const std::string& message,
               const std::string& file, uint32_t line)
            {
                ComponentEnumEntry entry = py_enum_to_entry(component);
                logger.log(severity, entry, message, file, line);
            },
            py::arg("severity"), py::arg("component"), py::arg("message"),
            py::arg("file") = "", py::arg("line") = 0);
}
