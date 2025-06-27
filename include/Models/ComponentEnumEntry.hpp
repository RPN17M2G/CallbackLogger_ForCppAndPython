#pragma once

#include <variant>
#include <typeindex>
#include <string>
#include <sstream>
#include <cstdint>
#include <functional>

class ComponentEnumEntry {
public:
    ComponentEnumEntry();
    ComponentEnumEntry(const std::variant<std::type_index, std::string>& type, uint32_t enum_value);

    ComponentEnumEntry(const ComponentEnumEntry& other);
    ComponentEnumEntry& operator=(const ComponentEnumEntry& other);

    /**
     * @brief Gets the type of the enum entry.
     *
     * @return The type as a variant (std::type_index or string).
     */
    const std::variant<std::type_index, std::string>& get_type() const;

    /**
     * @brief Sets the type of the enum entry.
     *
     * @param type The type as a string.
     */
    void set_type(const std::string& type);

    /**
     * @brief Sets the enum value.
     *
     * @param value The integer value of the enum.
     */
    void set_enum_value(const uint32_t value);

    /**
     * @brief Gets the enum value.
     *
     * @return The integer value of the enum.
     */
    uint32_t get_enum_value() const;

    /**
     * @brief Equality operator.
     *
     * @param other The other ComponentEnumEntry to compare.
     * @return True if equal, false otherwise.
     */
    bool operator==(const ComponentEnumEntry& other) const;

    /**
     * @brief Less-than operator.
     *
     * @param other The other ComponentEnumEntry to compare.
     * @return True if this is less than other.
     */
    bool operator<(const ComponentEnumEntry& other) const;

    /**
     * @brief Greater-than operator.
     *
     * @param other The other ComponentEnumEntry to compare.
     * @return True if this is greater than other.
     */
    bool operator>(const ComponentEnumEntry& other) const;

    /**
     * @brief Less-than or equal operator.
     *
     * @param other The other ComponentEnumEntry to compare.
     * @return True if this is less than or equal to other.
     */
    bool operator<=(const ComponentEnumEntry& other) const;

    /**
     * @brief Greater-than or equal operator.
     *
     * @param other The other ComponentEnumEntry to compare.
     * @return True if this is greater than or equal to other.
     */
    bool operator>=(const ComponentEnumEntry& other) const;

    /**
     * @brief Converts the entry to a string representation.
     *
     * @return String representation of the entry.
     */
    std::string to_string() const;

private:
    std::variant<std::type_index, std::string> type;
    uint32_t enum_value;

    friend struct ComponentEnumEntryHasher;
};

/**
 * @brief Hash functor for ComponentEnumEntry.
 */
struct ComponentEnumEntryHasher
{
    std::size_t operator()(const ComponentEnumEntry& entry) const
    {
        return std::hash<std::variant<std::type_index, std::string>>()(entry.type) ^ std::hash<uint32_t>()(entry.enum_value);
    }
};
