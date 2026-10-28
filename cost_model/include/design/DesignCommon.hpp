#ifndef DESIGN_COMMON_HPP
#define DESIGN_COMMON_HPP

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <iostream>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace design {

// Type definitions 
using String = std::string;
using IntType = int;
using FloatType = double;
using BoolType = bool;

// Default values
const String DEFAULT_STRING_VALUE = "";
const IntType DEFAULT_INT_VALUE = -1;
const FloatType DEFAULT_FLOAT_VALUE = -1.0;
const BoolType DEFAULT_BOOL_VALUE = false;

// Utility functions for string to bool conversion
inline bool StringToBool(const String& value) {
    String lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    return lowerValue == "true";
}

// Utility function for bool to string conversion
inline String BoolToString(bool value) {
    return value ? "True" : "False";
}

// Power function with Python math.pow semantics for the cases the cost model
// uses (yield^n with 0 <= yield <= 1 and n >= 0).
//
// Python's math.pow is the C library pow() on doubles, so std::pow on long
// double matches it to within a ulp. The previous implementation routed every
// call through a 50-digit boost::multiprecision type and sat on the per-move
// hot path (AssemblyYield), costing more than the whole floorplanner.
inline long double python_pow(double base, double exponent) {
    if (exponent < 0.0 && base == 0.0) {
        throw std::domain_error("0 cannot be raised to a negative power");
    }
    return std::pow(static_cast<long double>(base),
                    static_cast<long double>(exponent));
}

} // namespace design

#endif // DESIGN_COMMON_HPP 