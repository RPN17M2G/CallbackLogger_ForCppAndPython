#include "python_logger_types.hpp"

ComponentEnumEntry py_enum_to_entry(const py::object& enum_object)
{
    if (!py::hasattr(enum_object, "value"))
        throw std::runtime_error("Object is not a valid enum with a value attribute");

    uint32_t value = enum_object.attr("value").cast<uint32_t>();
    std::string py_enum_class_name = py::str(enum_object.attr("__class__").attr("__name__"));
    return ComponentEnumEntry{std::variant<std::type_index, std::string>{py_enum_class_name}, value};
}

void register_python_logger_types(py::module_& m)
{
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
}
