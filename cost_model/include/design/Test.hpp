#ifndef TEST_HPP
#define TEST_HPP

#include "DesignCommon.hpp"
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>
#include <optional>

namespace design {

/**
 * @class Test
 * @brief Represents a test process with its parameters
 * 
 * The class has the following attributes:
 *   name: The name of the test process.
 *   time_per_test_cycle: The time per test cycle.
 *   cost_per_second: The cost per second of testing.
 *   samples_per_input: The number of samples per input.
 *   test_self: Whether self-test is enabled.
 *   bb_self_pattern_count: The number of self-test patterns.
 *   bb_self_scan_chain_length: The length of the self-test scan chain.
 *   self_defect_coverage: The defect coverage of self-test.
 *   self_test_reuse: The reuse percentage of self-test.
 *   self_num_scan_chains: The number of scan chains for self-test.
 *   self_num_io_per_scan_chain: The number of IOs per scan chain for self-test.
 *   self_num_test_io_offset: The number of test IO offsets for self-test.
 *   self_test_failure_dist: The distribution of self-test failures.
 *   test_assembly: Whether assembly test is enabled.
 *   bb_assembly_pattern_count: The number of assembly test patterns.
 *   bb_assembly_scan_chain_length: The length of the assembly test scan chain.
 *   assembly_defect_coverage: The defect coverage of assembly test.
 *   assembly_test_reuse: The reuse percentage of assembly test.
 *   assembly_gate_flop_ratio: The gate to flop ratio for assembly test.
 *   assembly_num_scan_chains: The number of scan chains for assembly test.
 *   assembly_num_io_per_scan_chain: The number of IOs per scan chain for assembly test.
 *   assembly_num_test_io_offset: The number of test IO offsets for assembly test.
 *   assembly_test_failure_dist: The distribution of assembly test failures.
 *   static: A boolean set true when the test process is defined to prevent further changes.
 */
class Test {
public:
    /**
     * @brief Constructor for Test
     * 
     * @param name Name of the test process
     * @param time_per_test_cycle Time per test cycle in seconds
     * @param cost_per_second Cost per second of testing
     * @param samples_per_input Number of samples per input
     * @param test_self Whether self-test is enabled
     * @param bb_self_pattern_count Number of self-test patterns
     * @param bb_self_scan_chain_length Length of self-test scan chain
     * @param self_defect_coverage Defect coverage of self-test (0-1)
     * @param self_test_reuse Reuse percentage of self-test
     * @param self_num_scan_chains Number of scan chains for self-test
     * @param self_num_io_per_scan_chain Number of IOs per scan chain for self-test
     * @param self_num_test_io_offset Number of test IO offsets for self-test
     * @param self_test_failure_dist Distribution of self-test failures
     * @param test_assembly Whether assembly test is enabled
     * @param bb_assembly_pattern_count Number of assembly test patterns
     * @param bb_assembly_scan_chain_length Length of assembly test scan chain
     * @param assembly_defect_coverage Defect coverage of assembly test (0-1)
     * @param assembly_test_reuse Reuse percentage of assembly test
     * @param assembly_num_scan_chains Number of scan chains for assembly test
     * @param assembly_num_io_per_scan_chain Number of IOs per scan chain for assembly test
     * @param assembly_num_test_io_offset Number of test IO offsets for assembly test
     * @param assembly_test_failure_dist Distribution of assembly test failures
     * @param static_value Whether the test process is static (unchangeable)
     */
    Test(
        const String& name = DEFAULT_STRING_VALUE,
        FloatType time_per_test_cycle = DEFAULT_FLOAT_VALUE,
        FloatType cost_per_second = DEFAULT_FLOAT_VALUE,
        IntType samples_per_input = DEFAULT_INT_VALUE,
        const String& test_self = DEFAULT_STRING_VALUE,
        FloatType bb_self_pattern_count = DEFAULT_FLOAT_VALUE,
        FloatType bb_self_scan_chain_length = DEFAULT_FLOAT_VALUE,
        FloatType self_defect_coverage = DEFAULT_FLOAT_VALUE,
        FloatType self_test_reuse = DEFAULT_FLOAT_VALUE,
        IntType self_num_scan_chains = DEFAULT_INT_VALUE,
        IntType self_num_io_per_scan_chain = DEFAULT_INT_VALUE,
        IntType self_num_test_io_offset = DEFAULT_INT_VALUE,
        const String& self_test_failure_dist = DEFAULT_STRING_VALUE,
        const String& test_assembly = DEFAULT_STRING_VALUE,
        FloatType bb_assembly_pattern_count = DEFAULT_FLOAT_VALUE,
        FloatType bb_assembly_scan_chain_length = DEFAULT_FLOAT_VALUE,
        FloatType assembly_defect_coverage = DEFAULT_FLOAT_VALUE,
        FloatType assembly_test_reuse = DEFAULT_FLOAT_VALUE,
        IntType assembly_num_scan_chains = DEFAULT_INT_VALUE,
        IntType assembly_num_io_per_scan_chain = DEFAULT_INT_VALUE,
        IntType assembly_num_test_io_offset = DEFAULT_INT_VALUE,
        const String& assembly_test_failure_dist = DEFAULT_STRING_VALUE,
        BoolType static_value = true);

    // Getters and setters for all properties
    String GetName() const;
    IntType SetName(const String& value);

    FloatType GetTimePerTestCycle() const;
    IntType SetTimePerTestCycle(FloatType value);

    FloatType GetCostPerSecond() const;
    IntType SetCostPerSecond(FloatType value);

    IntType GetSamplesPerInput() const;
    IntType SetSamplesPerInput(IntType value);

    BoolType GetTestSelf() const;
    IntType SetTestSelf(const String& value);

    FloatType GetBbSelfPatternCount() const;
    IntType SetBbSelfPatternCount(FloatType value);

    FloatType GetBbSelfScanChainLength() const;
    IntType SetBbSelfScanChainLength(FloatType value);

    FloatType GetSelfDefectCoverage() const;
    IntType SetSelfDefectCoverage(FloatType value);

    FloatType GetSelfTestReuse() const;
    IntType SetSelfTestReuse(FloatType value);

    IntType GetSelfNumScanChains() const;
    IntType SetSelfNumScanChains(IntType value);

    IntType GetSelfNumIoPerScanChain() const;
    IntType SetSelfNumIoPerScanChain(IntType value);

    IntType GetSelfNumTestIoOffset() const;
    IntType SetSelfNumTestIoOffset(IntType value);

    String GetSelfTestFailureDist() const;
    IntType SetSelfTestFailureDist(const String& value);

    BoolType GetTestAssembly() const;
    IntType SetTestAssembly(const String& value);

    FloatType GetBbAssemblyPatternCount() const;
    IntType SetBbAssemblyPatternCount(FloatType value);

    FloatType GetBbAssemblyScanChainLength() const;
    IntType SetBbAssemblyScanChainLength(FloatType value);

    FloatType GetAssemblyDefectCoverage() const;
    IntType SetAssemblyDefectCoverage(FloatType value);

    FloatType GetAssemblyTestReuse() const;
    IntType SetAssemblyTestReuse(FloatType value);

    FloatType GetAssemblyGateFlopRatio() const;
    IntType SetAssemblyGateFlopRatio(FloatType value);

    IntType GetAssemblyNumScanChains() const;
    IntType SetAssemblyNumScanChains(IntType value);

    IntType GetAssemblyNumIoPerScanChain() const;
    IntType SetAssemblyNumIoPerScanChain(IntType value);

    IntType GetAssemblyNumTestIoOffset() const;
    IntType SetAssemblyNumTestIoOffset(IntType value);

    String GetAssemblyTestFailureDist() const;
    IntType SetAssemblyTestFailureDist(const String& value);

    BoolType GetStatic() const;
    IntType SetStatic(BoolType value);

    /**
     * @brief Check if all test parameters are defined
     * 
     * @return true if all parameters are defined, false otherwise
     */
    BoolType TestFullyDefined() const;

    /**
     * @brief Set the test process as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Convert test process to string representation
     * 
     * @return String representation of the test process
     */
    String ToString() const;
    
    /**
     * @brief Compute the cycles per pattern for testing
     * 
     * @param gate_count Number of gates
     * @param test_coverage Test coverage (0-1)
     * @param bb_pattern_count Pattern count
     * @param bb_scan_chain_length Scan chain length
     * @return Cycles per pattern
     */
    FloatType ComputeCyclesPerPattern(FloatType gate_count, FloatType test_coverage,
                                     FloatType bb_pattern_count, FloatType bb_scan_chain_length) const;
    
    /**
     * @brief Compute self test time for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Self test time in seconds
     */
    FloatType ComputeSelfTime(FloatType gate_count) const;
    
    /**
     * @brief Compute assembly test time for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Assembly test time in seconds
     */
    FloatType ComputeAssemblyTime(FloatType gate_count) const;
    
    /**
     * @brief Compute total test time for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Total test time in seconds
     */
    FloatType ComputeTotalTime(FloatType gate_count) const;
    
    /**
     * @brief Compute self test cost for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Self test cost
     */
    FloatType ComputeSelfCost(FloatType gate_count) const;
    
    /**
     * @brief Compute assembly test cost for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Assembly test cost
     */
    FloatType ComputeAssemblyCost(FloatType gate_count) const;
    
    /**
     * @brief Compute total test cost for a given number of gates
     * 
     * @param gate_count Number of gates
     * @return Total test cost
     */
    FloatType ComputeTotalCost(FloatType gate_count) const;
    
    /**
     * @brief Compute required IO pins for self test
     * 
     * @return Required IO pin count
     */
    IntType ComputeRequiredSelfIo() const;
    
    /**
     * @brief Compute required IO pins for assembly test
     * 
     * @return Required IO pin count
     */
    IntType ComputeRequiredAssemblyIo() const;
    
    /**
     * @brief Compute self-test yield for a given chip
     * 
     * @param chip Chip to test
     * @return Self-test yield
     */
    FloatType ComputeSelfTestYield(FloatType self_true_yield, const String& chip_name = "") const;
    
    /**
     * @brief Compute self-test quality for a given chip
     * 
     * @param chip Chip to test
     * @return Self-test quality
     */
    FloatType ComputeSelfQuality(FloatType self_true_yield, FloatType self_test_yield) const;
    
    /**
     * @brief Compute assembly test yield for a given chip
     * 
     * @param chip Chip to test
     * @return Assembly test yield
     */
    FloatType ComputeAssemblyTestYield(FloatType chip_true_yield) const;
    
    /**
     * @brief Compute assembly test quality for a given chip
     * 
     * @param chip Chip to test
     * @return Assembly test quality
     */
    FloatType ComputeAssemblyQuality(FloatType chip_true_yield, FloatType chip_test_yield) const;
    
    /**
     * @brief Compute self-test pattern count for a given chip
     * 
     * @param chip Chip to test
     * @return Self-test pattern count
     */
    FloatType ComputeSelfPatternCount(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const;
    
    /**
     * @brief Compute self-test scan chain length per mm^2 for a given chip
     * 
     * @param chip Chip to test
     * @return Self-test scan chain length per mm^2
     */
    FloatType ComputeSelfScanChainLengthPerMm2(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const;
    
    /**
     * @brief Compute self-test cost for a given chip
     * 
     * @param chip Chip to test
     * @return Self-test cost
     */
    FloatType ComputeSelfTestCost(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const;
    
    /**
     * @brief Calculate gate-to-flop ratio for a given chip
     * 
     * @param chip Chip to analyze
     * @return Gate-to-flop ratio
     */
    FloatType AssemblyGateFlopRatio(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const;
    
    /**
     * @brief Compute assembly test pattern count for a given chip
     * 
     * @param chip Chip to test
     * @return Assembly test pattern count
     */
    FloatType ComputeAssemblyPatternCount(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const;
    
    /**
     * @brief Compute assembly test scan chain length per mm^2 for a given chip
     * 
     * @param chip Chip to test
     * @return Assembly test scan chain length per mm^2
     */
    FloatType ComputeAssemblyScanChainLengthPerMm2(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const;
    
    /**
     * @brief Compute assembly test cost for a given chip
     * 
     * @param chip Chip to test
     * @return Assembly test cost
     */
    FloatType ComputeAssemblyTestCost(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const;
    
    /**
     * @brief Calculate number of test IOs required
     * 
     * @return Number of test IOs
     */
    FloatType NumTestIos() const;
    
    /**
     * @brief Calculate ATPG (Automatic Test Pattern Generation) cost for a given chip
     * 
     * @param chip Chip to test
     * @return ATPG cost
     */
    FloatType GetAtpgCost(FloatType core_area, FloatType assembly_core_area, 
                         FloatType self_gates_per_mm2, FloatType assembly_gates_per_mm2,
                         FloatType gate_flop_ratio) const;

private:
    // Private member variables (equivalent to Python's __variables)
    String name_;
    FloatType time_per_test_cycle_;
    FloatType cost_per_second_;
    IntType samples_per_input_;
    std::optional<BoolType>test_self_;
    FloatType bb_self_pattern_count_;
    FloatType bb_self_scan_chain_length_;
    FloatType self_defect_coverage_;
    FloatType self_test_reuse_;
    IntType self_num_scan_chains_;
    IntType self_num_io_per_scan_chain_;
    IntType self_num_test_io_offset_;
    String self_test_failure_dist_;
    std::optional<BoolType> test_assembly_;
    FloatType bb_assembly_pattern_count_;
    FloatType bb_assembly_scan_chain_length_;
    FloatType assembly_defect_coverage_;
    FloatType assembly_test_reuse_;
    FloatType assembly_gate_flop_ratio_;
    IntType assembly_num_scan_chains_;
    IntType assembly_num_io_per_scan_chain_;
    IntType assembly_num_test_io_offset_;
    String assembly_test_failure_dist_;
    BoolType static_;
};

// Allow streaming the Test object
std::ostream& operator<<(std::ostream& os, const Test& test);

} // namespace design

#endif // TEST_HPP 