#include "Models/ComponentEnumEntry.hpp"

ComponentEnumEntry::ComponentEnumEntry()
    : type(typeid(void)), enum_value(0) {}

ComponentEnumEntry::ComponentEnumEntry(const std::variant<std::type_index, std::string>& type, uint32_t enum_value)
    : type(type), enum_value(enum_value) {}

ComponentEnumEntry::ComponentEnumEntry(const ComponentEnumEntry& other)
    : type(other.type), enum_value(other.enum_value) {}

ComponentEnumEntry& ComponentEnumEntry::operator=(const ComponentEnumEntry& other) {
    if (this != &other) {
        type = other.type;
        enum_value = other.enum_value;
    }
    return *this;
}

const std::variant<std::type_index, std::string>& ComponentEnumEntry::get_type() const {
    return type;
}

uint32_t ComponentEnumEntry::get_enum_value() const {
    return enum_value;
}

bool ComponentEnumEntry::operator==(const ComponentEnumEntry& other) const {
    return (other.type == type) && (other.enum_value == enum_value);
}

bool ComponentEnumEntry::operator<(const ComponentEnumEntry& other) const {
    return (type < other.type) || (type == other.type && enum_value < other.enum_value);
}

bool ComponentEnumEntry::operator>(const ComponentEnumEntry& other) const {
    return (type > other.type) || (type == other.type && enum_value > other.enum_value);
}

bool ComponentEnumEntry::operator<=(const ComponentEnumEntry& other) const {
    return !(*this > other);
}

bool ComponentEnumEntry::operator>=(const ComponentEnumEntry& other) const {
    return !(*this < other);
}

std::string ComponentEnumEntry::to_string() const {
    std::ostringstream oss;
    std::string type_string = std::visit([](const auto& t) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(t)>, std::type_index>) {
                return t.name();
            } else {
                return t;
            }
        }, type);

    size_t position = 0;
    while ((position < type_string.size()) && std::isdigit(static_cast<unsigned char>(type_string[position]))) {
        ++position;
    }
    type_string = type_string.substr(position);

    oss << type_string << "#" << enum_value;
    return oss.str();
}

void ComponentEnumEntry::set_type(const std::string& type)
{
    this->type = type;
}

void ComponentEnumEntry::set_enum_value(const uint32_t value)
{
    this->enum_value = value;
}
