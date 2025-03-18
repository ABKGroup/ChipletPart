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
#include <boost/multiprecision/cpp_dec_float.hpp>

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

// Custom power function that mimics Python's math.pow behavior using Boost
inline long double python_pow(double base, double exponent) {
    // Use boost multiprecision for high precision calculations
    typedef boost::multiprecision::cpp_dec_float_50 float_type;
    float_type b = static_cast<float_type>(base);
    float_type e = static_cast<float_type>(exponent);
    
    // Special handling for integer exponents for better precision
    if (std::floor(exponent) == exponent) {
        float_type result = 1.0;
        float_type b_pow = b;
        long long exp = static_cast<long long>(exponent);
        
        // Handle negative exponents
        if (exp < 0) {
            if (base == 0) {
                throw std::domain_error("0 cannot be raised to a negative power");
            }
            b_pow = 1.0 / b;
            exp = -exp;
        }
        
        // Exponentiation by squaring for integer powers
        while (exp > 0) {
            if (exp & 1) {
                result *= b_pow;
            }
            b_pow *= b_pow;
            exp >>= 1;
        }
        
        return static_cast<long double>(result);
    }
    
    // For non-integer exponents, use boost's pow
    float_type result = boost::multiprecision::pow(b, e);
    return static_cast<long double>(result);
}

} // namespace design

#endif // DESIGN_COMMON_HPP 