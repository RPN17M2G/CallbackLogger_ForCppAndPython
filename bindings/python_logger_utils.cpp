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

namespace {

/// @brief Helper to convert a Python filter object to the appropriate C++ filter and call the given registration function.

/// @tparam RegisterFunc The logger registration function signature.
/// @param logger The logger instance.
/// @param safe_callback The callback to register.
/// @param filter The Python filter object.
/// @param register_func The logger registration function (member pointer or lambda).
/// @return The callback handle.
template <typename RegisterFunc>
uint32_t handle_register_callback(
        CallbackLogger& logger,
        const LogCallback& safe_callback,
        py::object filter,
        RegisterFunc register_func)
    {
        if (py::isinstance<py::dict>(filter))
        {
            std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> native_filter;
            for (std::pair<pybind11::handle, pybind11::handle> item : filter.cast<py::dict>())
            {
                ComponentEnumEntry entry = py_enum_to_entry(static_cast<const py::object &>(item.first));
                native_filter[entry] = item.second.cast<Severity>();
            }
            return register_func(native_filter);
        }
        else if (py::isinstance<py::set>(filter))
        {
            std::set<ComponentEnumEntry> native_set;
            for (pybind11::handle item : filter.cast<py::set>())
            {
                native_set.insert(py_enum_to_entry(py::reinterpret_borrow<py::object>(item)));
            }
            return register_func(native_set);
        }
        else if (py::isinstance<Severity>(filter))
        {
            return register_func(filter.cast<Severity>());
        }
        else if (py::hasattr(filter, "value"))
        {
            ComponentEnumEntry entry = py_enum_to_entry(filter);
            return register_func(entry);
        }
        else if (filter.is_none())
        {
            return register_func(Severity::Uninitialized);
        }
        else
        {
            throw std::runtime_error("Unsupported filter type for register callback");
        }
    }

}

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
                return handle_register_callback(
                    logger, safe_callback, filter,
                    [&](auto&& native_filter) {
                        return logger.register_function_callback(safe_callback, std::forward<decltype(native_filter)>(native_filter));
                    }
                );
            }, py::arg("callback"), py::arg("filter") = py::none())
        .def("register_file_callback",
            [](CallbackLogger& logger, const std::string& filename, py::object filter)
            {
                return handle_register_callback(
                    logger, nullptr, filter,
                    [&](auto&& native_filter) {
                        return logger.register_file_callback(filename, std::forward<decltype(native_filter)>(native_filter));
                    }
                );
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