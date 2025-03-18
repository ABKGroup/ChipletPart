#include "design/Test.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace design {

Test::Test(
    const String& name,
    FloatType time_per_test_cycle,
    FloatType cost_per_second,
    IntType samples_per_input,
    const String& test_self,
    FloatType bb_self_pattern_count,
    FloatType bb_self_scan_chain_length,
    FloatType self_defect_coverage,
    FloatType self_test_reuse,
    IntType self_num_scan_chains,
    IntType self_num_io_per_scan_chain,
    IntType self_num_test_io_offset,
    const String& self_test_failure_dist,
    const String& test_assembly,
    FloatType bb_assembly_pattern_count,
    FloatType bb_assembly_scan_chain_length,
    FloatType assembly_defect_coverage,
    FloatType assembly_test_reuse,
    IntType assembly_num_scan_chains,
    IntType assembly_num_io_per_scan_chain,
    IntType assembly_num_test_io_offset,
    const String& assembly_test_failure_dist,
    BoolType static_value)
    : static_(false)  // Initialize static_ to false first
{
    // Initialize all properties
    name_ = name;
    time_per_test_cycle_ = time_per_test_cycle;
    cost_per_second_ = cost_per_second;
    samples_per_input_ = samples_per_input;
    
    // Convert test_self string to bool
    if (!test_self.empty()) {
        String lowerValue = test_self;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        test_self_ = (lowerValue == "true");
    } else {
        test_self_ = false;
    }
    
    bb_self_pattern_count_ = bb_self_pattern_count;
    bb_self_scan_chain_length_ = bb_self_scan_chain_length;
    self_defect_coverage_ = self_defect_coverage;
    self_test_reuse_ = self_test_reuse;
    self_num_scan_chains_ = self_num_scan_chains;
    self_num_io_per_scan_chain_ = self_num_io_per_scan_chain;
    self_num_test_io_offset_ = self_num_test_io_offset;
    self_test_failure_dist_ = self_test_failure_dist;
    
    // Convert test_assembly string to bool
    if (!test_assembly.empty()) {
        String lowerValue = test_assembly;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        test_assembly_ = (lowerValue == "true");
    } else {
        test_assembly_ = false;
    }
    
    bb_assembly_pattern_count_ = bb_assembly_pattern_count;
    bb_assembly_scan_chain_length_ = bb_assembly_scan_chain_length;
    assembly_defect_coverage_ = assembly_defect_coverage;
    assembly_test_reuse_ = assembly_test_reuse;
    assembly_gate_flop_ratio_ = 0.0;  // Default to 0.0, will be calculated later
    assembly_num_scan_chains_ = assembly_num_scan_chains;
    assembly_num_io_per_scan_chain_ = assembly_num_io_per_scan_chain;
    assembly_num_test_io_offset_ = assembly_num_test_io_offset;
    assembly_test_failure_dist_ = assembly_test_failure_dist;
    
    // Set static_ last, after all properties have been initialized
    static_ = static_value;
    
    // If name is empty, set to non-static (matching Python implementation)
    if (name_.empty()) {
        std::cout << "Warning: Test not fully defined. Setting non-static." << std::endl;
        static_ = false;
        std::cout << "Test has name " << name_ << "." << std::endl;
    }
}

String Test::GetName() const {
    return name_;
}

IntType Test::SetName(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Test name must be a string." << std::endl;
            return 1;
        } else {
            name_ = value;
            return 0;
        }
    }
}

FloatType Test::GetTimePerTestCycle() const {
    return time_per_test_cycle_;
}

IntType Test::SetTimePerTestCycle(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Time per test cycle must be nonnegative." << std::endl;
            return 1;
        } else {
            time_per_test_cycle_ = value;
            return 0;
        }
    }
}

FloatType Test::GetCostPerSecond() const {
    return cost_per_second_;
}

IntType Test::SetCostPerSecond(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Cost per second must be nonnegative." << std::endl;
            return 1;
        } else {
            cost_per_second_ = value;
            return 0;
        }
    }
}

IntType Test::GetSamplesPerInput() const {
    return samples_per_input_;
}

IntType Test::SetSamplesPerInput(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Samples per input must be nonnegative." << std::endl;
            return 1;
        } else {
            samples_per_input_ = value;
            return 0;
        }
    }
}

BoolType Test::GetTestSelf() const {
    return test_self_.value_or(false);
}

IntType Test::SetTestSelf(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Test self must be a string either \"True\" or \"true\"." << std::endl;
            return 1;
        } else {
            String lowerValue = value;
            std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (lowerValue == "true") {
                test_self_ = true;
            } else {
                test_self_ = false;
            }
            return 0;
        }
    }
}

FloatType Test::GetBbSelfPatternCount() const {
    return bb_self_pattern_count_;
}

IntType Test::SetBbSelfPatternCount(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_self_pattern_count_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB self pattern count must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_self_pattern_count_ = value;
            return 0;
        }
    }
}

FloatType Test::GetBbSelfScanChainLength() const {
    return bb_self_scan_chain_length_;
}

IntType Test::SetBbSelfScanChainLength(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_self_scan_chain_length_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB self scan chain length must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_self_scan_chain_length_ = value;
            return 0;
        }
    }
}

FloatType Test::GetSelfDefectCoverage() const {
    return self_defect_coverage_;
}

IntType Test::SetSelfDefectCoverage(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Self defect coverage must be between 0 and 1." << std::endl;
            return 1;
        } else {
            self_defect_coverage_ = value;
            return 0;
        }
    }
}

FloatType Test::GetSelfTestReuse() const {
    return self_test_reuse_;
}

IntType Test::SetSelfTestReuse(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Self test reuse must be nonnegative." << std::endl;
            return 1;
        } else {
            self_test_reuse_ = value;
            return 0;
        }
    }
}

IntType Test::GetSelfNumScanChains() const {
    return self_num_scan_chains_;
}

IntType Test::SetSelfNumScanChains(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Self num scan chains must be nonnegative." << std::endl;
            return 1;
        } else {
            self_num_scan_chains_ = value;
            return 0;
        }
    }
}

IntType Test::GetSelfNumIoPerScanChain() const {
    return self_num_io_per_scan_chain_;
}

IntType Test::SetSelfNumIoPerScanChain(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Self num IO per scan chain must be nonnegative." << std::endl;
            return 1;
        } else {
            self_num_io_per_scan_chain_ = value;
            return 0;
        }
    }
}

IntType Test::GetSelfNumTestIoOffset() const {
    return self_num_test_io_offset_;
}

IntType Test::SetSelfNumTestIoOffset(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Self num test IO offset must be nonnegative." << std::endl;
            return 1;
        } else {
            self_num_test_io_offset_ = value;
            return 0;
        }
    }
}

String Test::GetSelfTestFailureDist() const {
    return self_test_failure_dist_;
}

IntType Test::SetSelfTestFailureDist(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Self test failure dist must be a string." << std::endl;
            return 1;
        } else {
            self_test_failure_dist_ = value;
            return 0;
        }
    }
}

BoolType Test::GetTestAssembly() const {
    return test_assembly_.value_or(false);
}

IntType Test::SetTestAssembly(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Test assembly must be a string either \"True\" or \"true\"." << std::endl;
            return 1;
        } else {
            String lowerValue = value;
            std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (lowerValue == "true") {
                test_assembly_ = true;
            } else {
                test_assembly_ = false;
            }
            return 0;
        }
    }
}

FloatType Test::GetBbAssemblyPatternCount() const {
    return bb_assembly_pattern_count_;
}

IntType Test::SetBbAssemblyPatternCount(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_assembly_pattern_count_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB assembly pattern count must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_assembly_pattern_count_ = value;
            return 0;
        }
    }
}

FloatType Test::GetBbAssemblyScanChainLength() const {
    return bb_assembly_scan_chain_length_;
}

IntType Test::SetBbAssemblyScanChainLength(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_assembly_scan_chain_length_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB assembly scan chain length must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_assembly_scan_chain_length_ = value;
            return 0;
        }
    }
}

FloatType Test::GetAssemblyDefectCoverage() const {
    return assembly_defect_coverage_;
}

IntType Test::SetAssemblyDefectCoverage(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0 || value > 1) {
            std::cout << "Error: Assembly defect coverage must be between 0 and 1." << std::endl;
            return 1;
        } else {
            assembly_defect_coverage_ = value;
            return 0;
        }
    }
}

FloatType Test::GetAssemblyTestReuse() const {
    return assembly_test_reuse_;
}

IntType Test::SetAssemblyTestReuse(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Assembly test reuse must be nonnegative." << std::endl;
            return 1;
        } else {
            assembly_test_reuse_ = value;
            return 0;
        }
    }
}

FloatType Test::GetAssemblyGateFlopRatio() const {
    return assembly_gate_flop_ratio_;
}

IntType Test::SetAssemblyGateFlopRatio(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Assembly gate flop ratio must be nonnegative." << std::endl;
            return 1;
        } else {
            assembly_gate_flop_ratio_ = value;
            return 0;
        }
    }
}

IntType Test::GetAssemblyNumScanChains() const {
    return assembly_num_scan_chains_;
}

IntType Test::SetAssemblyNumScanChains(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Assembly num scan chains must be nonnegative." << std::endl;
            return 1;
        } else {
            assembly_num_scan_chains_ = value;
            return 0;
        }
    }
}

IntType Test::GetAssemblyNumIoPerScanChain() const {
    return assembly_num_io_per_scan_chain_;
}

IntType Test::SetAssemblyNumIoPerScanChain(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Assembly num IO per scan chain must be nonnegative." << std::endl;
            return 1;
        } else {
            assembly_num_io_per_scan_chain_ = value;
            return 0;
        }
    }
}

IntType Test::GetAssemblyNumTestIoOffset() const {
    return assembly_num_test_io_offset_;
}

IntType Test::SetAssemblyNumTestIoOffset(IntType value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Assembly num test IO offset must be nonnegative." << std::endl;
            return 1;
        } else {
            assembly_num_test_io_offset_ = value;
            return 0;
        }
    }
}

String Test::GetAssemblyTestFailureDist() const {
    return assembly_test_failure_dist_;
}

IntType Test::SetAssemblyTestFailureDist(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static testing." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Assembly test failure dist must be a string." << std::endl;
            return 1;
        } else {
            assembly_test_failure_dist_ = value;
            return 0;
        }
    }
}

FloatType Test::ComputeCyclesPerPattern(FloatType gate_count, FloatType test_coverage, 
                                         FloatType bb_pattern_count, FloatType bb_scan_chain_length) const {
    if (gate_count <= 0 || bb_pattern_count <= 0 || bb_scan_chain_length <= 0) {
        return 0;
    }
    
    FloatType cycles = gate_count * test_coverage / bb_pattern_count / bb_scan_chain_length;
    return cycles;
}

FloatType Test::ComputeSelfTime(FloatType gate_count) const {
    if (!test_self_.value_or(false)) {
        return 0;
    }
    
    if (bb_self_pattern_count_ <= 0 || bb_self_scan_chain_length_ <= 0) {
        std::cout << "Error: BB self pattern count and scan chain length must be positive to calculate self test time." << std::endl;
        return 0;
    }
    
    FloatType cycles = ComputeCyclesPerPattern(gate_count, self_defect_coverage_, 
                                               bb_self_pattern_count_, bb_self_scan_chain_length_);
    FloatType time = cycles * time_per_test_cycle_ / self_test_reuse_;
    return time;
}

FloatType Test::ComputeAssemblyTime(FloatType gate_count) const {
    if (!test_assembly_.value_or(false)) {
        return 0;
    }
    
    if (bb_assembly_pattern_count_ <= 0 || bb_assembly_scan_chain_length_ <= 0) {
        std::cout << "Error: BB assembly pattern count and scan chain length must be positive to calculate assembly test time." << std::endl;
        return 0;
    }
    
    FloatType cycles = ComputeCyclesPerPattern(gate_count, assembly_defect_coverage_, 
                                               bb_assembly_pattern_count_, bb_assembly_scan_chain_length_);
    FloatType time = cycles * time_per_test_cycle_ / assembly_test_reuse_;
    return time;
}

FloatType Test::ComputeTotalTime(FloatType gate_count) const {
    FloatType self_time = ComputeSelfTime(gate_count);
    FloatType assembly_time = ComputeAssemblyTime(gate_count);
    
    return self_time + assembly_time;
}

FloatType Test::ComputeSelfCost(FloatType gate_count) const {
    if (!test_self_.value_or(false)) {
        return 0.0;
    }
    return ComputeSelfTime(gate_count) * cost_per_second_;
}

FloatType Test::ComputeAssemblyCost(FloatType gate_count) const {
    if (!test_assembly_.value_or(false)) {
        return 0.0;
    }
    return ComputeAssemblyTime(gate_count) * cost_per_second_;
}

FloatType Test::ComputeTotalCost(FloatType gate_count) const {
    FloatType cost = 0.0;
    if (test_self_.value_or(false)) {
        cost += ComputeSelfCost(gate_count);
    }
    if (test_assembly_.value_or(false)) {
        cost += ComputeAssemblyCost(gate_count);
    }
    return cost;
}

IntType Test::ComputeRequiredSelfIo() const {
    if (!test_self_.value_or(false)) {
        return 0;
    }
    
    // If scan chain parameters are not defined, return 0
    if (self_num_scan_chains_ <= 0 || self_num_io_per_scan_chain_ <= 0) {
        return 0;
    }
    
    // Required IO = (num_scan_chains * num_io_per_scan_chain) + num_test_io_offset
    IntType io = (self_num_scan_chains_ * self_num_io_per_scan_chain_) + self_num_test_io_offset_;
    return io;
}

IntType Test::ComputeRequiredAssemblyIo() const {
    if (!test_assembly_.value_or(false)) {
        return 0;
    }
    
    // If scan chain parameters are not defined, return 0
    if (assembly_num_scan_chains_ <= 0 || assembly_num_io_per_scan_chain_ <= 0) {
        return 0;
    }
    
    // Required IO = (num_scan_chains * num_io_per_scan_chain) + num_test_io_offset
    IntType io = (assembly_num_scan_chains_ * assembly_num_io_per_scan_chain_) + assembly_num_test_io_offset_;
    return io;
}

BoolType Test::TestFullyDefined() const {
    // Match Python implementation exactly
    if (name_.empty() ||
        time_per_test_cycle_ == DEFAULT_FLOAT_VALUE ||
        cost_per_second_ == DEFAULT_FLOAT_VALUE ||
        samples_per_input_ == DEFAULT_INT_VALUE ||
        !test_self_.has_value() /* Check if optional has a value */ ||
        self_defect_coverage_ == DEFAULT_FLOAT_VALUE ||
        self_test_reuse_ == DEFAULT_FLOAT_VALUE ||
        self_num_scan_chains_ == DEFAULT_INT_VALUE ||
        self_num_io_per_scan_chain_ == DEFAULT_INT_VALUE ||
        self_num_test_io_offset_ == DEFAULT_INT_VALUE ||
        self_test_failure_dist_.empty() ||
        !test_assembly_.has_value() /* Check if optional has a value */ ||
        assembly_defect_coverage_ == DEFAULT_FLOAT_VALUE ||
        assembly_test_reuse_ == DEFAULT_FLOAT_VALUE ||
        assembly_num_scan_chains_ == DEFAULT_INT_VALUE ||
        assembly_num_io_per_scan_chain_ == DEFAULT_INT_VALUE ||
        assembly_num_test_io_offset_ == DEFAULT_INT_VALUE ||
        assembly_test_failure_dist_.empty()) {
        return false;
    } else {
        return true;
    }
}

String Test::ToString() const {
    std::ostringstream oss;
    oss << "Test: " << name_ << std::endl;
    oss << "Time_per_test_cycle: " << time_per_test_cycle_ << std::endl;
    oss << "Cost_per_second: " << cost_per_second_ << std::endl;
    oss << "Samples_per_input: " << samples_per_input_ << std::endl;
    oss << "Test_self: " << (test_self_.value_or(false) ? "True" : "False") << std::endl;
    oss << "Bb_self_pattern_count: " << bb_self_pattern_count_ << std::endl;
    oss << "Bb_self_scan_chain_length: " << bb_self_scan_chain_length_ << std::endl;
    oss << "Self_defect_coverage: " << self_defect_coverage_ << std::endl;
    oss << "Self_test_reuse: " << self_test_reuse_ << std::endl;
    oss << "Self_num_scan_chains: " << self_num_scan_chains_ << std::endl;
    oss << "Self_num_io_per_scan_chain: " << self_num_io_per_scan_chain_ << std::endl;
    oss << "Self_num_test_io_offset: " << self_num_test_io_offset_ << std::endl;
    oss << "Self_test_failure_dist: " << self_test_failure_dist_ << std::endl;
    oss << "Test_assembly: " << (test_assembly_.value_or(false) ? "True" : "False") << std::endl;
    oss << "bb_assembly_pattern_count: " << bb_assembly_pattern_count_ << std::endl;
    oss << "bb_assembly_scan_chain_length: " << bb_assembly_scan_chain_length_ << std::endl;
    oss << "Assembly_defect_coverage: " << assembly_defect_coverage_ << std::endl;
    oss << "Assembly_test_reuse: " << assembly_test_reuse_ << std::endl;
    oss << "Assembly_num_scan_chains: " << assembly_num_scan_chains_ << std::endl;
    oss << "Assembly_num_io_per_scan_chain: " << assembly_num_io_per_scan_chain_ << std::endl;
    oss << "Assembly_num_test_io_offset: " << assembly_num_test_io_offset_ << std::endl;
    oss << "Assembly_test_failure_dist: " << assembly_test_failure_dist_ << std::endl;
    oss << "Static: " << (static_ ? "True" : "False") << std::endl;
    
    return oss.str();
}

IntType Test::SetStatic() {
    if (!TestFullyDefined()) {
        std::cout << "Error: Attempt to set test static without defining all parameters. Exiting..." << std::endl;
        std::cout << ToString() << std::endl;
        std::exit(1);
    }
    static_ = true;
    return 0;
}

BoolType Test::GetStatic() const {
    return static_;
}

IntType Test::SetStatic(BoolType value) {
    static_ = value;
    return 0;
}

// Implement operator<<
std::ostream& operator<<(std::ostream& os, const Test& test) {
    os << test.ToString();
    return os;
}

FloatType Test::NumTestIos() const {
    IntType num_ios = 0;
    if (test_self_.value_or(false)) {
        num_ios = std::max(num_ios, ComputeRequiredSelfIo());
    }
    if (test_assembly_.value_or(false)) {
        num_ios = std::max(num_ios, ComputeRequiredAssemblyIo());
    }
    return static_cast<FloatType>(num_ios);
}

FloatType Test::ComputeSelfScanChainLengthPerMm2(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const {
    if (bb_self_scan_chain_length_ != DEFAULT_FLOAT_VALUE) {
        return bb_self_scan_chain_length_;
    }
    
    FloatType num_gates = core_area * self_gates_per_mm2;
    FloatType num_flops = num_gates / gate_flop_ratio;
    FloatType scan_length = num_flops / self_num_scan_chains_;
    
    return scan_length;
}

FloatType Test::ComputeSelfTestYield(FloatType self_true_yield, const String& chip_name) const {
    if (!test_self_.value_or(false)) {
        return 1.0;
    }

    FloatType test_yield;
    test_yield = 1.0 - (1.0 - self_true_yield) * self_defect_coverage_;
    return test_yield;
}

FloatType Test::ComputeSelfQuality(FloatType self_true_yield, FloatType self_test_yield) const {
    if (!test_self_.value_or(false)) {
        return 1.0;
    }

    FloatType self_quality;
    if (self_test_yield == 0.0) {
        self_quality = 0.0;
    } else {
        self_quality = self_true_yield / self_test_yield;
    }
    return self_quality;
}

FloatType Test::ComputeAssemblyTestYield(FloatType chip_true_yield) const {
    if (!test_assembly_.value_or(false)) {
        return 1.0;
    }

    FloatType assembly_test_yield;
    assembly_test_yield = 1.0 - (1.0 - chip_true_yield) * assembly_defect_coverage_;
    
    return assembly_test_yield;
}

FloatType Test::AssemblyGateFlopRatio(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const {
    if (assembly_gate_flop_ratio_ > 0.0) {
        return assembly_gate_flop_ratio_;
    }
    
    FloatType num_gates = assembly_core_area * assembly_gates_per_mm2;
    if (num_gates <= 0.0) {
        return 1.0;  // Default value
    }
    
    return num_gates;
}

FloatType Test::ComputeAssemblyPatternCount(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const {
    if (bb_assembly_pattern_count_ != DEFAULT_FLOAT_VALUE) {
        return bb_assembly_pattern_count_;
    }
    
    FloatType gate_flop_ratio = AssemblyGateFlopRatio(assembly_core_area, assembly_gates_per_mm2);
    FloatType num_gates = assembly_core_area * assembly_gates_per_mm2;
    FloatType num_flops = num_gates / gate_flop_ratio;
    FloatType scan_length = num_flops / assembly_num_scan_chains_;
    
    FloatType pattern_count = 2 * scan_length * std::sqrt(scan_length);
    pattern_count *= assembly_defect_coverage_;
    pattern_count *= (1.0 - assembly_test_reuse_);
    
    return pattern_count;
}

FloatType Test::ComputeSelfPatternCount(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const {
    if (bb_self_pattern_count_ != DEFAULT_FLOAT_VALUE) {
        return bb_self_pattern_count_;
    }
    
    FloatType num_gates = core_area * self_gates_per_mm2;
    FloatType num_flops = num_gates / gate_flop_ratio;
    FloatType scan_length = num_flops / self_num_scan_chains_;
    
    FloatType pattern_count = 2 * scan_length * std::sqrt(scan_length);
    pattern_count *= self_defect_coverage_;
    pattern_count *= (1.0 - self_test_reuse_);

    return pattern_count;
}

FloatType Test::ComputeAssemblyScanChainLengthPerMm2(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const {
    if (bb_assembly_scan_chain_length_ != DEFAULT_FLOAT_VALUE) {
        return bb_assembly_scan_chain_length_;
    }
    
    FloatType gate_flop_ratio = AssemblyGateFlopRatio(assembly_core_area, assembly_gates_per_mm2);
    FloatType num_gates = assembly_core_area * assembly_gates_per_mm2;
    FloatType num_flops = num_gates / gate_flop_ratio;
    FloatType scan_length = num_flops / assembly_num_scan_chains_;
    
    return scan_length;
}

FloatType Test::ComputeAssemblyTestCost(FloatType assembly_core_area, FloatType assembly_gates_per_mm2) const {
    FloatType test_cost;
    if (!test_assembly_.value_or(false)) {
        test_cost = 0.0;
    } else {
        FloatType assembly_pattern_count = ComputeAssemblyPatternCount(assembly_core_area, assembly_gates_per_mm2);
        FloatType assembly_scan_chain_length = ComputeAssemblyScanChainLengthPerMm2(assembly_core_area, assembly_gates_per_mm2);
        
        test_cost = assembly_core_area * time_per_test_cycle_ * cost_per_second_ * 
                   (assembly_pattern_count + samples_per_input_) * assembly_scan_chain_length;
        
        // Derating Factor Ignored for Now
        FloatType derating_factor = 1.0;
        test_cost = derating_factor * test_cost;
    }
    return test_cost;
}

FloatType Test::GetAtpgCost(FloatType core_area, FloatType assembly_core_area, 
                           FloatType self_gates_per_mm2, FloatType assembly_gates_per_mm2,
                           FloatType gate_flop_ratio) const {
    return 0.0;
}

FloatType Test::ComputeSelfTestCost(FloatType core_area, FloatType self_gates_per_mm2, FloatType gate_flop_ratio) const {
    FloatType test_cost;
    if (!test_self_.value_or(false)) {
        test_cost = 0.0;
    } else {
        test_cost = core_area * time_per_test_cycle_ * cost_per_second_ * 
                   (ComputeSelfPatternCount(core_area, self_gates_per_mm2, gate_flop_ratio) + samples_per_input_) * 
                   ComputeSelfScanChainLengthPerMm2(core_area, self_gates_per_mm2, gate_flop_ratio);
        
        // Derating Factor Ignored for Now - just like in Python
        FloatType derating_factor = 1.0;
        test_cost = derating_factor * test_cost;
    }
    return test_cost;
}

FloatType Test::ComputeAssemblyQuality(FloatType chip_true_yield, FloatType chip_test_yield) const {
    if (!test_assembly_.value_or(false)) {
        return 1.0;
    }

    FloatType assembly_quality;
    if (chip_test_yield == 0.0) {
        assembly_quality = 0.0;
    } else {
        assembly_quality = chip_true_yield / chip_test_yield;
    }
    return assembly_quality;
}

} // namespace design 