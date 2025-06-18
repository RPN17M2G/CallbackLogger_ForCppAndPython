#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <string>

#include "CallbackLogger.hpp"
#include "LoggingFunctionsExamples.hpp"

namespace py = pybind11;

PYBIND11_MODULE(pycallbacklogger, m) {

     m.def("function_that_logs", &function_that_logs, py::arg("logger"));
     m.def("startup_function_that_logs", &startup_function_that_logs, py::arg("logger"));

    py::enum_<Severity>(m, "Severity")
        .value("Uninitialized", Severity::Uninitialized)
        .value("Debug", Severity::Debug)
        .value("Info", Severity::Info)
        .value("Warning", Severity::Warning)
        .value("Error", Severity::Error)
        .value("Fatal", Severity::Fatal)
        .export_values();

    py::enum_<Component>(m, "Component")
        .value("S", Component::S)
        .value("M", Component::M)
        .value("P", Component::P)
        .export_values();

    py::class_<LogEntry>(m, "LogEntry")
        .def_readonly("severity", &LogEntry::severity)
        .def_readonly("component", &LogEntry::component)
        .def_readonly("message", &LogEntry::message)
        .def_readonly("file", &LogEntry::file)
        .def_readonly("line", &LogEntry::line)
        .def_readonly("timestamp", &LogEntry::timestamp);

    py::class_<CallbackLogger>(m, "CallbackLogger")
        .def(py::init<>())
        .def("register_function_callback",
             py::overload_cast<const LogCallback&, const std::unordered_map<Component, Severity>&>(&CallbackLogger::register_function_callback),
             py::arg("callback"), py::arg("filter") = std::unordered_map<Component, Severity>())
        .def("register_function_callback",
             py::overload_cast<const LogCallback&, const std::set<Component>&>(&CallbackLogger::register_function_callback),
             py::arg("callback"), py::arg("component_filter"))
        .def("register_function_callback",
             py::overload_cast<const LogCallback&, Severity>(&CallbackLogger::register_function_callback),
             py::arg("callback"), py::arg("min_severity"))
        .def("register_function_callback",
             py::overload_cast<const LogCallback&, Component>(&CallbackLogger::register_function_callback),
             py::arg("callback"), py::arg("component"))
        .def("register_file_callback",
             py::overload_cast<const std::string&, Component>(&CallbackLogger::register_file_callback),
             py::arg("filename"), py::arg("component"))
        .def("register_file_callback",
             py::overload_cast<const std::string&, const std::unordered_map<Component, Severity>&>(&CallbackLogger::register_file_callback),
             py::arg("filename"), py::arg("filter") = std::unordered_map<Component, Severity>())
        .def("register_file_callback",
             py::overload_cast<const std::string&, const std::set<Component>&>(&CallbackLogger::register_file_callback),
             py::arg("filename"), py::arg("component_filter"))
        .def("register_file_callback",
             py::overload_cast<const std::string&, Severity>(&CallbackLogger::register_file_callback),
             py::arg("filename"), py::arg("min_severity"))
        .def("unregister_function_callback", &CallbackLogger::unregister_function_callback, py::arg("handle"))
        .def("unregister_file_callback", &CallbackLogger::unregister_file_callback, py::arg("handle"))
        .def("log", &CallbackLogger::log,
             py::arg("severity"), py::arg("component"), py::arg("message"),
             py::arg("file") = "", py::arg("line") = 0);
}
