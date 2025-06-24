#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <string>
#include "CallbackLogger.hpp"

namespace py = pybind11;

ComponentEnumEntry py_enum_to_entry(const py::object& enum_obj) {
    if (!py::hasattr(enum_obj, "value"))
        throw std::runtime_error("Object is not a valid enum with a value attribute");

    uint32_t value = enum_obj.attr("value").cast<uint32_t>();
    std::string py_enum_class_name = py::str(enum_obj.attr("__class__").attr("__name__"));
    return ComponentEnumEntry{std::variant<std::type_index, std::string>{py_enum_class_name}, value};
}

// Derived class that registers shutdown to atexit
class PyCallbackLogger : public CallbackLogger {
public:
    PyCallbackLogger() : CallbackLogger(0) { // Always single-threaded for Python
        py::object self = py::cast(this);
        py::module_::import("atexit").attr("register")(py::cpp_function([self]() {
            py::gil_scoped_acquire gil;
            //self.attr("shutdown")();
        }));
    }
};

PYBIND11_MODULE(pycallbacklogger, m) {
    py::enum_<Severity>(m, "Severity")
        .value("Uninitialized", Severity::Uninitialized)
        .value("Debug", Severity::Debug)
        .value("Info", Severity::Info)
        .value("Warning", Severity::Warning)
        .value("Error", Severity::Error)
        .value("Fatal", Severity::Fatal)
        .export_values();

    py::class_<LogEntry>(m, "LogEntry")
        .def_readonly("severity", &LogEntry::severity)
        .def_readonly("component", &LogEntry::component)
        .def_readonly("message", &LogEntry::message)
        .def_readonly("file", &LogEntry::file)
        .def_readonly("line", &LogEntry::line)
        .def_readonly("timestamp", &LogEntry::timestamp);

    py::class_<ComponentEnumEntry>(m, "ComponentEnumEntry")
        .def(py::init<>())
        .def("get_type", &ComponentEnumEntry::get_type)
        .def("get_enum_value", &ComponentEnumEntry::get_enum_value)
        .def("set_type", &ComponentEnumEntry::set_type)
        .def("set_enum_value", &ComponentEnumEntry::set_enum_value)
        .def("__str__", &ComponentEnumEntry::to_string)
        .def("__repr__", &ComponentEnumEntry::to_string)
        .def("__eq__", &ComponentEnumEntry::operator==)
        .def("__lt__", &ComponentEnumEntry::operator<)
        .def("__gt__", &ComponentEnumEntry::operator>)
        .def("__le__", &ComponentEnumEntry::operator<=)
        .def("__ge__", &ComponentEnumEntry::operator>=);

    // Register base class first (not exposed to Python directly)
    py::class_<CallbackLogger>(m, "CallbackLoggerBase")
        .def(py::init<>())
        .def("shutdown", &CallbackLogger::shutdown)
        .def("unregister_function_callback", &CallbackLogger::unregister_function_callback, py::arg("handle"))
        .def("unregister_file_callback", &CallbackLogger::unregister_file_callback, py::arg("handle"));

    // Expose PyCallbackLogger as CallbackLogger
    py::class_<PyCallbackLogger, CallbackLogger>(m, "CallbackLogger")
        .def(py::init<>())
        .def("shutdown", &CallbackLogger::shutdown)
        .def("register_function_callback",
            [](CallbackLogger& logger, py::function py_callback, py::object filter) {
                LogCallback safe_callback = [py_callback](const LogEntry& entry) {
                    if (!Py_IsInitialized()) return;
                    try {
                        py::gil_scoped_acquire gil;
                        py_callback(entry);
                    } catch (const std::exception &e) {
                        py::print("[!] Exception in Python callback:", e.what());
                    } catch (...) {
                        py::print("[!] Unknown exception in Python callback.");
                    }
                };

                if (py::isinstance<py::dict>(filter)) {
                    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> native_filter;
                    for (auto item : filter.cast<py::dict>()) {
                        ComponentEnumEntry entry = py_enum_to_entry(static_cast<const py::object &>(item.first));
                        native_filter[entry] = item.second.cast<Severity>();
                    }
                    return logger.register_function_callback(safe_callback, native_filter);
                } else if (py::isinstance<py::set>(filter)) {
                    std::set<ComponentEnumEntry> native_set;
                    for (auto item : filter.cast<py::set>()) {
                        native_set.insert(py_enum_to_entry(py::reinterpret_borrow<py::object>(item)));
                    }
                    return logger.register_function_callback(safe_callback, native_set);
                } else if (py::hasattr(filter, "value")) {
                    ComponentEnumEntry entry = py_enum_to_entry(filter);
                    return logger.register_function_callback(safe_callback, entry);
                } else if (py::isinstance<Severity>(filter)) {
                    return logger.register_function_callback(safe_callback, filter.cast<Severity>());
                } else if (filter.is_none()) {
                    return logger.register_function_callback(safe_callback, Severity::Uninitialized);
                } else {
                    throw std::runtime_error("Unsupported filter type for register_function_callback");
                }
            }, py::arg("callback"), py::arg("filter") = py::none())
        .def("register_file_callback",
            [](CallbackLogger& logger, const std::string& filename, py::object filter) {
                if (py::isinstance<py::dict>(filter)) {
                    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> native_filter;
                    for (auto item : filter.cast<py::dict>()) {
                        ComponentEnumEntry entry = py_enum_to_entry(static_cast<const py::object &>(item.first));
                        native_filter[entry] = item.second.cast<Severity>();
                    }
                    return logger.register_file_callback(filename, native_filter);
                } else if (py::isinstance<py::set>(filter)) {
                    std::set<ComponentEnumEntry> native_set;
                    for (auto item : filter.cast<py::set>()) {
                        native_set.insert(py_enum_to_entry(static_cast<const py::object &>(item)));
                    }
                    return logger.register_file_callback(filename, native_set);
                } else if (py::hasattr(filter, "value")) {
                    ComponentEnumEntry entry = py_enum_to_entry(filter);
                    return logger.register_file_callback(filename, entry);
                } else if (py::isinstance<Severity>(filter)) {
                    return logger.register_file_callback(filename, filter.cast<Severity>());
                } else if (filter.is_none()) {
                    return logger.register_file_callback(filename, Severity::Uninitialized);
                } else {
                    throw std::runtime_error("Unsupported filter type for register_file_callback");
                }
            }, py::arg("filename"), py::arg("filter") = py::none())
        .def("log",
            [](CallbackLogger& logger, Severity severity, py::object component, const std::string& message,
               const std::string& file, uint32_t line) {
                ComponentEnumEntry entry = py_enum_to_entry(component);
                logger.log(severity, entry, message, file, line);
            },
            py::arg("severity"), py::arg("component"), py::arg("message"),
            py::arg("file") = "", py::arg("line") = 0);
}