#ifndef IO_HPP
#define IO_HPP

#include "DesignCommon.hpp"
#include <iostream>

namespace design {

/**
 * @class IO
 * @brief Represents an IO interface with its parameters
 * 
 * The class has the following attributes:
 *   type: The type of the IO.
 *   rx_area: The area of the receiver part of the IO in mm^2.
 *   tx_area: The area of the transmitter part of the IO in mm^2.
 *   shoreline: The shoreline of the IO in mm.
 *   bandwidth: The bandwidth of the IO in Gb/s.
 *   wire_count: The number of wires used by the IO.
 *   bidirectional: Whether the IO is bidirectional.
 *   energy_per_bit: The energy per bit of the IO in pJ/bit.
 *   reach: The reach of the IO in mm.
 *   static: Whether the IO definition is finalized.
 */
class IO {
public:
    /**
     * @brief Constructor for IO
     */
    IO(
        const String& type = DEFAULT_STRING_VALUE,
        FloatType rx_area = DEFAULT_FLOAT_VALUE,
        FloatType tx_area = DEFAULT_FLOAT_VALUE,
        FloatType shoreline = DEFAULT_FLOAT_VALUE,
        FloatType bandwidth = DEFAULT_FLOAT_VALUE,
        IntType wire_count = DEFAULT_INT_VALUE,
        const String& bidirectional = DEFAULT_STRING_VALUE,
        FloatType energy_per_bit = DEFAULT_FLOAT_VALUE,
        FloatType reach = DEFAULT_FLOAT_VALUE,
        BoolType static_value = true);

    // Getters and setters
    String GetType() const;
    IntType SetType(const String& value);

    FloatType GetRxArea() const;
    IntType SetRxArea(FloatType value);

    FloatType GetTxArea() const;
    IntType SetTxArea(FloatType value);

    FloatType GetShoreline() const;
    IntType SetShoreline(FloatType value);

    FloatType GetBandwidth() const;
    IntType SetBandwidth(FloatType value);

    IntType GetWireCount() const;
    IntType SetWireCount(IntType value);

    BoolType GetBidirectional() const;
    IntType SetBidirectional(const String& value);

    FloatType GetEnergyPerBit() const;
    IntType SetEnergyPerBit(FloatType value);

    FloatType GetReach() const;
    IntType SetReach(FloatType value);

    BoolType GetStatic() const;
    IntType SetStatic(BoolType value);

    /**
     * @brief Set the IO as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Check if the IO is fully defined
     * 
     * @return true if fully defined, false otherwise
     */
    BoolType IoFullyDefined() const;

    /**
     * @brief Convert IO to string representation
     * 
     * @return String representation of IO
     */
    String ToString() const;

    // Operator overload for stream output
    friend std::ostream& operator<<(std::ostream& os, const IO& io);

private:
    // Private member variables
    String type_;
    FloatType rx_area_;
    FloatType tx_area_;
    FloatType shoreline_;
    FloatType bandwidth_;
    IntType wire_count_;
    BoolType bidirectional_;
    FloatType energy_per_bit_;
    FloatType reach_;
    BoolType static_;
};

} // namespace design

#endif // IO_HPP 