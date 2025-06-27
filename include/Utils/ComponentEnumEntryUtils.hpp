#pragma once

#include "Models/ComponentEnumEntry.hpp"

/**
 * @brief Converts an enum value to a ComponentEnumEntry.
 *
 * @tparam EnumT Enum type.
 * @param value Enum value.
 * @return ComponentEnumEntry representing the enum value.
 */
template <typename EnumT>
ComponentEnumEntry make_component_entry(EnumT value) {
    static_assert(std::is_enum<EnumT>::value, "EnumT must be an enum type");
    return ComponentEnumEntry{std::type_index(typeid(EnumT)), static_cast<uint32_t>(value)};
}
