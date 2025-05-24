#include "design/IO.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace design {

IO::IO(
    const String& type,
    FloatType rx_area,
    FloatType tx_area,
    FloatType shoreline,
    FloatType bandwidth,
    IntType wire_count,
    const String& bidirectional,
    FloatType energy_per_bit,
    FloatType reach,
    BoolType static_value)
{
    // In Python, first initialize static to False
    static_ = false;
    
    // Directly set properties without validation, matching Python implementation
    type_ = type;
    rx_area_ = rx_area;
    tx_area_ = tx_area;
    shoreline_ = shoreline;
    bandwidth_ = bandwidth;
    wire_count_ = wire_count;
    
    // For bidirectional, convert string to bool (similar to Python logic)
    if (!bidirectional.empty()) {
        String lowerValue = bidirectional;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        bidirectional_ = (lowerValue == "true");
    } else {
        bidirectional_ = false;
    }
    
    // Duplicate assignment of bidirectional to match the Python code exactly
    if (!bidirectional.empty()) {
        String lowerValue = bidirectional;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        bidirectional_ = (lowerValue == "true");
    } else {
        bidirectional_ = false;
    }
    
    energy_per_bit_ = energy_per_bit;
    reach_ = reach;
    
    // Set static last
    static_ = static_value;
 
    // Check if fully defined and update static if needed
    if (!IoFullyDefined()) {
        std::cout << "Warning: IO not fully defined, setting to non-static." << std::endl;
        static_ = false;
        //std::cout << ToString() << std::endl;
    }
}

String IO::GetType() const {
    return type_;
}

IntType IO::SetType(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: IO type must be a string." << std::endl;
            return 1;
        } else {
            type_ = value;
            return 0;
        }
    }
}

FloatType IO::GetRxArea() const {
    return rx_area_;
}

IntType IO::SetRxArea(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: RX area must be nonnegative." << std::endl;
            return 1;
        } else {
            rx_area_ = value;
            return 0;
        }
    }
}

FloatType IO::GetTxArea() const {
    return tx_area_;
}

IntType IO::SetTxArea(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: TX area must be nonnegative." << std::endl;
            return 1;
        } else {
            tx_area_ = value;
            return 0;
        }
    }
}

FloatType IO::GetShoreline() const {
    return shoreline_;
}

IntType IO::SetShoreline(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Shoreline must be nonnegative." << std::endl;
            return 1;
        } else {
            shoreline_ = value;
            return 0;
        }
    }
}

FloatType IO::GetBandwidth() const {
    return bandwidth_;
}

IntType IO::SetBandwidth(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Bandwidth must be nonnegative." << std::endl;
            return 1;
        } else {
            bandwidth_ = value;
            return 0;
        }
    }
}

IntType IO::GetWireCount() const {
    return wire_count_;
}

IntType IO::SetWireCount(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Wire count must be nonnegative." << std::endl;
            return 1;
        } else {
            wire_count_ = value;
            return 0;
        }
    }
}

BoolType IO::GetBidirectional() const {
    return bidirectional_;
}

IntType IO::SetBidirectional(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Bidirectional must be a string. (True or False)" << std::endl;
            return 1;
        } else {
            // Convert to lowercase for case-insensitive comparison, matching Python's .lower()
            String lowerValue = value;
            std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), 
                          [](unsigned char c) { return std::tolower(c); });
            
            if (lowerValue == "true") {
                bidirectional_ = true;
            } else {
                bidirectional_ = false;
            }
            return 0;
        }
    }
}

FloatType IO::GetEnergyPerBit() const {
    return energy_per_bit_;
}

IntType IO::SetEnergyPerBit(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Energy per bit must be nonnegative." << std::endl;
            return 1;
        } else {
            energy_per_bit_ = value;
            return 0;
        }
    }
}

FloatType IO::GetReach() const {
    return reach_;
}

IntType IO::SetReach(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static IO." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Reach must be nonnegative." << std::endl;
            return 1;
        } else {
            reach_ = value;
            return 0;
        }
    }
}

BoolType IO::GetStatic() const {
    return static_;
}

IntType IO::SetStatic(BoolType value) {
    static_ = value;
    return 0;
}

IntType IO::SetStatic() {
    if (!IoFullyDefined()) {
        std::cout << "Error: Attempt to set IO static without defining all parameters. Exiting..." << std::endl;
        std::cout << ToString() << std::endl;
        std::exit(1);
    }
    static_ = true;
    return 0;
}

BoolType IO::IoFullyDefined() const {
    // Match the Python implementation exactly - checking for None/default values
    if (type_.empty() ||
        rx_area_ == DEFAULT_FLOAT_VALUE ||
        tx_area_ == DEFAULT_FLOAT_VALUE ||
        shoreline_ == DEFAULT_FLOAT_VALUE ||
        bandwidth_ == DEFAULT_FLOAT_VALUE ||
        wire_count_ == DEFAULT_INT_VALUE ||
        energy_per_bit_ == DEFAULT_FLOAT_VALUE ||
        reach_ == DEFAULT_FLOAT_VALUE) {
        std::cout << "rx_area_: " << rx_area_ << " default: " << DEFAULT_FLOAT_VALUE << std::endl;
        std::cout << "tx_area_: " << tx_area_ << " default: " << DEFAULT_FLOAT_VALUE << std::endl;
        std::cout << "shoreline_: " << shoreline_ << " default: " << DEFAULT_FLOAT_VALUE << std::endl;
        std::cout << "bandwidth_: " << bandwidth_ << " default: " << DEFAULT_FLOAT_VALUE << std::endl;
        std::cout << "wire_count_: " << wire_count_ << " default: " << DEFAULT_INT_VALUE << std::endl;
        std::cout << "energy_per_bit_: " << energy_per_bit_ << "default: " << DEFAULT_FLOAT_VALUE << std::endl;
        std::cout << "reach_: " << reach_ << " default: " << DEFAULT_FLOAT_VALUE << std::endl;
        return false;
    } else {
        return true;
    }
}

String IO::ToString() const {
    // Match the Python __str__ implementation exactly
    String return_str = "IO Type: " + type_;
    return_str += "\n\r\tRX Area: " + std::to_string(rx_area_);
    return_str += "\n\r\tTX Area: " + std::to_string(tx_area_);
    return_str += "\n\r\tShoreline: " + std::to_string(shoreline_);
    return_str += "\n\r\tBandwidth: " + std::to_string(bandwidth_);
    return_str += "\n\r\tWire Count: " + std::to_string(wire_count_);
    return_str += "\n\r\tBidirectional: " + BoolToString(bidirectional_);
    return_str += "\n\r\tEnergy Per Bit: " + std::to_string(energy_per_bit_);
    return_str += "\n\r\tReach: " + std::to_string(reach_);
    return_str += "\n\r\tStatic: " + BoolToString(static_);
    return return_str;
}

std::ostream& operator<<(std::ostream& os, const IO& io) {
    os << io.ToString();
    return os;
}

} // namespace design 