#include "python_logger_types.hpp"

ComponentEnumEntry py_enum_to_entry(const py::object& enum_object)
{
    if (!py::hasattr(enum_object, "value"))
        throw std::runtime_error("Object is not a valid enum with a value attribute");

    uint32_t value = enum_object.attr("value").cast<uint32_t>();
    std::string py_enum_class_name = py::str(enum_object.attr("__class__").attr("__name__"));
    std::string py_enum_module_name = py::str(enum_object.attr("__class__").attr("__module__"));
    return ComponentEnumEntry{std::variant<std::type_index, std::string>{py_enum_module_name + MODULE_CLASS_DELIMITER + py_enum_class_name}, value};
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
            .def_property_readonly("component", [](const LogEntry& entry)
            {
                const ComponentEnumEntry& component = entry.component;
                const std::variant<std::type_index, std::string>& type_variant = component.get_type();

                if (std::holds_alternative<std::string>(type_variant))
                {
                    const std::string& component_string = std::get<std::string>(type_variant);
                    size_t module_class_delimiter_position = component_string.find(MODULE_CLASS_DELIMITER);
                    if (module_class_delimiter_position == std::string::npos)
                        throw std::runtime_error("[!] Invalid enum type string format (expected module and class data)");
                    
                    std::string module_name = component_string.substr(0, module_class_delimiter_position);
                    std::string class_name = component_string.substr(module_class_delimiter_position + std::strlen(MODULE_CLASS_DELIMITER));
                    uint32_t value = component.get_enum_value();

                    py::object py_module = py::module_::import(module_name.c_str());
                    py::object enum_class = py_module.attr(class_name.c_str());//
                    if (!py::hasattr(enum_class, "__members__"))
                        throw std::runtime_error("[!] Enum class '" + class_name + "' not found in module '" + module_name + "'");
                    return enum_class(value);
                }
                else if (std::holds_alternative<std::type_index>(type_variant)) // A cpp enum
                {
                    return py::object(py::int_(component.get_enum_value()));
                }
                else
                {
                    throw std::runtime_error("[!] Unknown enum type in log entry!");
                }
            })
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