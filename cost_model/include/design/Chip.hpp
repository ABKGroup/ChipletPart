#ifndef CHIP_HPP
#define CHIP_HPP

#include "DesignCommon.hpp"
#include "WaferProcess.hpp"
#include "IO.hpp"
#include "Layer.hpp"
#include "Assembly.hpp"
#include "Test.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <pugixml.hpp>

namespace design {

/**
 * @class Chip
 * @brief Represents a chip with its parameters and hierarchy
 * 
 * The class has the following attributes:
 *   name: The name of the chip.
 *   core_area: The area of the core in mm^2.
 *   aspect_ratio: The aspect ratio of the chip.
 *   x_location: The x location of the chip (in a stackup).
 *   y_location: The y location of the chip (in a stackup).
 *   bb_area: The black box area of the chip.
 *   bb_cost: The black box cost of the chip.
 *   bb_quality: The black box quality of the chip.
 *   bb_power: The black box power of the chip.
 *   fraction_memory: The fraction of the chip that is memory.
 *   fraction_logic: The fraction of the chip that is logic.
 *   fraction_analog: The fraction of the chip that is analog.
 *   gate_flop_ratio: The gate to flop ratio of the chip.
 *   reticle_share: The reticle share of the chip.
 *   buried: Whether the chip is buried (e.g., a bridge die in EMIB).
 *   assembly_process: The assembly process used for the chip.
 *   test_process: The test process used for the chip.
 *   stackup: The stackup of layers used for the chip.
 *   wafer_process: The wafer process used for the chip.
 *   core_voltage: The core voltage of the chip.
 *   power: The power consumption of the chip.
 *   quantity: The quantity of chips produced.
 *   chips: The child chips in the hierarchy.
 *   static: A boolean set true when the chip is defined to prevent further changes.
 */
class Chip : public std::enable_shared_from_this<Chip> {
public:
    /**
     * @brief Construct a new Chip object
     * 
     * @param filename Path to XML file
     * @param etree XML element tree (if filename is not provided)
     * @param parent_chip Parent chip
     * @param wafer_process_list List of available wafer processes
     * @param assembly_process_list List of available assembly processes
     * @param test_process_list List of available test processes
     * @param layers List of available layers
     * @param ios List of available IOs
     * @param adjacency_matrix_definitions Definitions of adjacency matrices
     * @param average_bandwidth_utilization Average bandwidth utilization matrix
     * @param block_names Names of blocks
     * @param static_value Whether the chip is static (unchangeable)
     */
    Chip(
        const String& filename = DEFAULT_STRING_VALUE,
        void* etree = nullptr,
        std::shared_ptr<Chip> parent_chip = nullptr,
        const std::vector<WaferProcess>* wafer_process_list = nullptr,
        const std::vector<Assembly>* assembly_process_list = nullptr,
        const std::vector<Test>* test_process_list = nullptr,
        const std::vector<Layer>* layers = nullptr,
        const std::vector<IO>* ios = nullptr,
        const std::map<String, std::vector<std::vector<IntType>>>* adjacency_matrix_definitions = nullptr,
        const std::map<String, std::vector<std::vector<FloatType>>>* average_bandwidth_utilization = nullptr,
        const std::vector<String>* block_names = nullptr,
        BoolType static_value = false);

    /**
     * @brief Initialize the Chip object after construction
     * 
     * This method completes the initialization of the Chip by setting up Test functions
     * and computing yields and costs. It should be called after the Chip object has been
     * constructed and wrapped in a shared_ptr.
     */
    void Initialize();

    // Getters and setters for all properties
    String GetName() const;
    IntType SetName(const String& value);

    FloatType GetCoreArea() const;
    IntType SetCoreArea(FloatType value);

    FloatType GetAspectRatio() const;
    IntType SetAspectRatio(FloatType value);

    FloatType GetXLocation() const;
    IntType SetXLocation(FloatType value);

    FloatType GetYLocation() const;
    IntType SetYLocation(FloatType value);

    FloatType GetBbArea() const;
    IntType SetBbArea(FloatType value);

    FloatType GetBbCost() const;
    IntType SetBbCost(FloatType value);

    FloatType GetBbQuality() const;
    IntType SetBbQuality(FloatType value);

    FloatType GetBbPower() const;
    IntType SetBbPower(FloatType value);

    FloatType GetFractionMemory() const;
    IntType SetFractionMemory(FloatType value);

    FloatType GetFractionLogic() const;
    IntType SetFractionLogic(FloatType value);

    FloatType GetFractionAnalog() const;
    IntType SetFractionAnalog(FloatType value);

    FloatType GetGateFlopRatio() const;
    IntType SetGateFlopRatio(FloatType value);

    FloatType GetReticleShare() const;
    IntType SetReticleShare(FloatType value);

    BoolType GetBuried() const;
    IntType SetBuried(const String& value);

    std::vector<std::shared_ptr<Chip>> GetChips() const;
    IntType SetChips(const std::vector<std::shared_ptr<Chip>>& value);

    std::shared_ptr<Assembly> GetAssemblyProcess() const;
    IntType SetAssemblyProcess(const std::shared_ptr<Assembly>& value);

    std::shared_ptr<Test> GetTestProcess() const;
    IntType SetTestProcess(const std::shared_ptr<Test>& value);

    std::vector<Layer> GetStackup() const;
    IntType SetStackup(const std::vector<Layer>& value);

    std::shared_ptr<WaferProcess> GetWaferProcess() const;
    IntType SetWaferProcess(const std::shared_ptr<WaferProcess>& value);

    FloatType GetCoreVoltage() const;
    IntType SetCoreVoltage(FloatType value);

    FloatType GetPower() const;
    IntType SetPower(FloatType value);

    long long GetQuantity() const;
    IntType SetQuantity(long long value);

    BoolType GetStatic() const;
    IntType SetStatic(BoolType value);

    // Hierarchical getter/setter functions
    FloatType GetStackPower() const;
    IntType SetStackPower(FloatType value);

    FloatType GetIoPower() const;
    IntType SetIoPower(FloatType value);

    FloatType GetTotalPower() const;
    IntType SetTotalPower(FloatType value);

    FloatType GetNreDesignCost() const;
    IntType SetNreDesignCost();

    IntType SetArea();

    FloatType GetSelfTrueYield() const;
    void SetSelfTrueYield(FloatType value);

    FloatType GetSelfTestYield() const;
    void SetSelfTestYield(FloatType value);

    FloatType GetSelfQuality() const;
    void SetSelfQuality(FloatType value);

    FloatType GetChipTrueYield() const;
    void SetChipTrueYield(FloatType value);

    FloatType GetChipTestYield() const;
    void SetChipTestYield(FloatType value);

    FloatType GetQuality() const;
    void SetQuality(FloatType value);

    FloatType GetSelfCost() const;
    void SetSelfCost(FloatType value);

    FloatType GetCost() const;
    void SetCost(FloatType value);

    void SetLayerApprox(); 
    void ResetLayerApprox();

    /**
     * @brief Set the chip as static (unchangeable)
     * 
     * @return 0 on success, 1 on failure
     */
    IntType SetStatic();

    /**
     * @brief Convert chip to string representation
     * 
     * @return String representation of the chip
     */
    String ToString() const;
    
    /**
     * @brief Compute power for all chips in the stack
     * 
     * @return Total power consumption
     */
    FloatType ComputeStackPower() const;
    
    /**
     * @brief Find a process by name in a list of processes
     * 
     * @param process_name Name of the process to find
     * @param process_list List of processes to search
     * @return Found process or null
     */
    template<typename T>
    std::shared_ptr<T> FindProcess(const String& process_name, const std::vector<T>& process_list) const;
    
    /**
     * @brief Find a wafer process by name
     * 
     * @param wafer_process_name Name of the wafer process
     * @param wafer_process_list List of wafer processes
     * @return Found wafer process
     */
    std::shared_ptr<WaferProcess> FindWaferProcess(const String& wafer_process_name, const std::vector<WaferProcess>& wafer_process_list) const;
    
    /**
     * @brief Find an assembly process by name
     * 
     * @param assembly_process_name Name of the assembly process
     * @param assembly_process_list List of assembly processes
     * @return Found assembly process
     */
    std::shared_ptr<Assembly> FindAssemblyProcess(const String& assembly_process_name, const std::vector<Assembly>& assembly_process_list) const;
    
    /**
     * @brief Find a test process by name
     * 
     * @param test_process_name Name of the test process
     * @param test_process_list List of test processes
     * @return Found test process
     */
    std::shared_ptr<Test> FindTestProcess(const String& test_process_name, const std::vector<Test>& test_process_list) const;
    
    /**
     * @brief Build a stackup from a string representation
     * 
     * @param stackup_string String representation of the stackup
     * @param layers Available layers
     * @return Built stackup
     */
    std::vector<Layer> BuildStackup(const String& stackup_string, const std::vector<Layer>& layers) const;
    
    /**
     * @brief Get a list of all chip names in the hierarchy
     * 
     * @return List of chip names
     */
    std::vector<String> GetChipList() const;
    
    /**
     * @brief Get the signal power for a list of blocks
     * 
     * @param internal_block_list List of internal block names
     * @return Signal power
     */
    FloatType GetSignalPower(const std::vector<String>& internal_block_list) const;
    
    /**
     * @brief Get the signal count for a list of blocks
     * 
     * @param internal_block_list List of internal block names
     * @return Pair of signal count and map of signals by reach
     */
    std::pair<IntType, std::map<String, IntType>> GetSignalCount(const std::vector<String>& internal_block_list) const;
    
    /**
     * @brief Get the signal count for all child chips
     * 
     * @return Signal count
     */
    IntType GetChipsSignalCount() const;
    
    /**
     * @brief Get the number of gates per mm^2 for self-test
     * 
     * @return Gates per mm^2
     */
    FloatType GetSelfGatesPerMm2() const;
    
    /**
     * @brief Get the number of gates per mm^2 for assembly test
     * 
     * @return Gates per mm^2
     */
    FloatType GetAssemblyGatesPerMm2() const;
    
    /**
     * @brief Compute layer-aware yield
     * 
     * @return Layer-aware yield
     */
    FloatType ComputeLayerAwareYield() const;
    
    /**
     * @brief Compute the yield of the chip
     * 
     * @return Chip yield
     */
    FloatType ComputeChipYield() const;
    
    /**
     * @brief Compute self cost
     * 
     * @return Self cost
     */
    FloatType ComputeSelfCost() const;
    
    /**
     * @brief Compute total cost
     * 
     * @return Total cost
     */
    FloatType ComputeCost() const;
    
    /**
     * @brief Get the parent chip
     * 
     * @return Parent chip
     */
    std::shared_ptr<Chip> GetParentChip() const;
    
    /**
     * @brief Get the assembly core area
     * 
     * @return Assembly core area in mm^2
     */
    FloatType GetAssemblyCoreArea() const;
    
    /**
     * @brief Get the number of child chips
     * 
     * @return Number of child chips
     */
    IntType GetChipsLen() const;
    
    /**
     * @brief Set chip definitions
     * 
     * @param chip_definitions Chip definitions
     * @return 0 on success, 1 on failure
     */
    IntType SetChipDefinitions(const std::map<String, String>& chip_definitions);
    
    /**
     * @brief Get the wafer diameter from the wafer process
     * 
     * @return Wafer diameter in mm
     */
    FloatType GetWaferDiameter() const;
    
    /**
     * @brief Get the edge exclusion from the wafer process
     * 
     * @return Edge exclusion in mm
     */
    FloatType GetEdgeExclusion() const;
    
    /**
     * @brief Get the wafer process yield from the wafer process
     * 
     * @return Wafer process yield
     */
    FloatType GetWaferProcessYield() const;
    
    /**
     * @brief Get the reticle X dimension from the wafer process
     * 
     * @return Reticle X dimension in mm
     */
    FloatType GetReticleX() const;
    
    /**
     * @brief Get the reticle Y dimension from the wafer process
     * 
     * @return Reticle Y dimension in mm
     */
    FloatType GetReticleY() const;
    
    /**
     * @brief Get the total area of the chip
     * 
     * @return Total area in mm^2
     */
    FloatType GetArea() const;
    
    /**
     * @brief Get the stacked die area
     * 
     * @return Stacked die area in mm^2
     */
    FloatType GetStackedDieArea() const;
    
    /**
     * @brief Compute front-end NRE cost
     * 
     * @return Front-end NRE cost
     */
    FloatType ComputeNreFrontEndCost() const;
    
    /**
     * @brief Compute back-end NRE cost
     * 
     * @return Back-end NRE cost
     */
    FloatType ComputeNreBackEndCost() const;
    
    /**
     * @brief Print the chip description
     */
    void PrintDescription() const;
    
    /**
     * @brief Helper method to calculate expanded area with margins
     * 
     * @param area Base area
     * @param margin Margin to add
     * @param aspect_ratio Aspect ratio (optional)
     * @return Expanded area
     */
    FloatType ExpandedArea(FloatType area, FloatType margin, FloatType aspect_ratio = 0.0) const;
    
    /**
     * @brief Get the number of power pads
     * 
     * @return Number of power pads
     */
    IntType GetPowerPads() const;
    
    /**
     * @brief Get the pad area
     * 
     * @return Pad area in mm^2
     */
    FloatType GetPadArea() const;
    
    /**
     * @brief Compute the area of the chip
     * 
     * @return Computed area in mm^2
     */
    FloatType ComputeArea() const;
    
    /**
     * @brief Get the IO area
     * 
     * @return IO area in mm^2
     */
    FloatType GetIoArea() const;
    
    /**
     * @brief Compute the number of reticles required
     * 
     * @param area Area to compute reticles for
     * @return A pair with the number of reticles and the number of stitches
     */
    std::pair<IntType, IntType> ComputeNumberReticles(FloatType area) const;
    
    /**
     * @brief Calculate quality yield based on component chips
     * 
     * @return Quality yield
     */
    FloatType QualityYield() const;
    
    /**
     * @brief Get chips with their counts
     * 
     * @return Map of chip names to counts
     */
    std::map<String, IntType> GetChipsWithCount() const;
    
    /**
     * @brief Calculate wafer area efficiency
     * 
     * @return Wafer area efficiency
     */
    FloatType WaferAreaEff() const;
    
    /**
     * @brief Compute NRE cost
     * 
     * @return NRE cost
     */
    FloatType ComputeNreCost() const;
    
    /**
     * @brief Compute NRE design cost (sum of front-end and back-end costs)
     * 
     * @return NRE design cost
     */
    FloatType ComputeNreDesignCost() const;
    
    /**
     * @brief Get mask cost
     * 
     * @return Mask cost
     */
    FloatType GetMaskCost() const;
    
    /**
     * @brief Compute die cost
     * 
     * @return Die cost
     */
    FloatType ComputeDieCost() const;
    
    /**
     * @brief Compute self perfect yield cost
     * 
     * @return Self perfect yield cost
     */
    FloatType ComputeSelfPerfectYieldCost() const;
    
    /**
     * @brief Compute perfect yield cost
     * 
     * @return Perfect yield cost
     */
    FloatType ComputePerfectYieldCost() const;
    
    /**
     * @brief Compute the scrap cost for the chip
     * @return The scrap cost
     */
    FloatType ComputeScrapCost() const;
    
    /**
     * @brief Compute total non-scrap cost
     * 
     * @return Total non-scrap cost
     */
    FloatType ComputeTotalNonScrapCost() const;
    
    /**
     * @brief Compute total cost (including all components)
     * 
     * @return Total cost
     */
    FloatType ComputeTotalCost() const;
    
    /**
     * @brief Get chips at a specific index
     * 
     * @param index Index of the chip to retrieve
     * @return Chip at the specified index
     */
    std::shared_ptr<Chip> GetChipsIndex(IntType index) const;
    
    /**
     * @brief Get the layer-aware cost for the chip
     * @return The layer-aware cost
     */
    FloatType GetLayerAwareCost() const;
    
    // Operator overload for stream output
    friend std::ostream& operator<<(std::ostream& os, const Chip& chip);

    /**
     * @brief Create a Chip object using a factory method to ensure proper initialization
     * 
     * @param filename The XML file to read (if etree is null)
     * @param etree The XML tree to use (if filename is empty)
     * @param parent_chip The parent chip (nullptr for top-level)
     * @param wafer_process_list The list of available wafer processes
     * @param assembly_process_list The list of available assembly processes
     * @param test_process_list The list of available test processes
     * @param layers The list of available layers
     * @param ios The list of available IOs
     * @param adjacency_matrix_definitions The adjacency matrix definitions
     * @param average_bandwidth_utilization The average bandwidth utilization
     * @param block_names The list of block names
     * @param static_value Whether the chip is static
     * @return std::shared_ptr<Chip> A properly initialized Chip object
     */
    static std::shared_ptr<Chip> Create(
        const String& filename,
        void* etree,
        std::shared_ptr<Chip> parent_chip,
        const std::vector<WaferProcess>* wafer_process_list,
        const std::vector<Assembly>* assembly_process_list,
        const std::vector<Test>* test_process_list,
        const std::vector<Layer>* layers,
        const std::vector<IO>* ios,
        const std::map<String, std::vector<std::vector<IntType>>>* adjacency_matrix_definitions,
        const std::map<String, std::vector<std::vector<FloatType>>>* average_bandwidth_utilization,
        const std::vector<String>* block_names,
        BoolType static_value,
        BoolType approx_state = false
    );

private:
    // Private member variables (equivalent to Python's __variables)
    String name_;
    FloatType core_area_;
    FloatType area_ = DEFAULT_FLOAT_VALUE;
    FloatType aspect_ratio_;
    FloatType x_location_;
    FloatType y_location_;
    FloatType bb_area_;
    FloatType bb_cost_;
    FloatType bb_quality_;
    FloatType bb_power_;
    FloatType fraction_memory_;
    FloatType fraction_logic_;
    FloatType fraction_analog_;
    FloatType gate_flop_ratio_;
    FloatType reticle_share_;
    BoolType buried_;
    std::vector<std::shared_ptr<Chip>> chips_;
    std::shared_ptr<Assembly> assembly_process_;
    std::shared_ptr<Test> test_process_;
    std::vector<Layer> stackup_;
    std::shared_ptr<WaferProcess> wafer_process_;
    FloatType core_voltage_;
    FloatType power_;
    long long quantity_;
    BoolType static_;
    
    // Additional member variables for hierarchical chip properties
    std::shared_ptr<Chip> parent_chip_;
    std::map<String, std::vector<std::vector<IntType>>> global_adjacency_matrix_;
    std::map<String, std::vector<std::vector<FloatType>>> average_bandwidth_utilization_;
    std::vector<String> block_names_;
    std::vector<std::shared_ptr<IO>> io_list_;
    
    // Computed properties
    FloatType stack_power_;
    FloatType io_power_;
    FloatType total_power_;
    FloatType nre_design_cost_;
    FloatType self_true_yield_;
    FloatType self_test_yield_;
    FloatType self_quality_;
    FloatType chip_true_yield_;
    FloatType chip_test_yield_;
    FloatType quality_;
    FloatType self_cost_;
    FloatType cost_;

    /**
     * @brief Initialize child chips safely after the parent chip is fully constructed
     * 
     * @param root The XML node containing the child chips
     * @param wafer_process_list The list of available wafer processes
     * @param assembly_process_list The list of available assembly processes
     * @param test_process_list The list of available test processes
     * @param layers The list of available layers
     * @param ios The list of available IOs
     * @param adjacency_matrix_definitions The adjacency matrix definitions
     * @param average_bandwidth_utilization The average bandwidth utilization
     * @param block_names The list of block names
     * @param static_value Whether the child chips are static
     */
    void InitializeChildChips(
        const pugi::xml_node& root,
        const std::vector<WaferProcess>* wafer_process_list,
        const std::vector<Assembly>* assembly_process_list,
        const std::vector<Test>* test_process_list,
        const std::vector<Layer>* layers,
        const std::vector<IO>* ios,
        const std::map<String, std::vector<std::vector<IntType>>>* adjacency_matrix_definitions,
        const std::map<String, std::vector<std::vector<FloatType>>>* average_bandwidth_utilization,
        const std::vector<String>* block_names,
        BoolType static_value
    );
};

} // namespace design

#endif // CHIP_HPP 