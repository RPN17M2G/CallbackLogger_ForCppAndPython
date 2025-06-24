#pragma once

#include <string>
#include <array>
#include <cstdint>

#include "Models/Severity.hpp"

/**
 * @brief Converts a Severity enum to its string representation.
 *
 * @param severity The severity level to convert.
 * @return String representation of the severity.
 */
std::string to_string(Severity severity);
