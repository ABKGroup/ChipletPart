#include "design/Chip.hpp"
#include "design/Layer.hpp"
#include "design/WaferProcess.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include "pugixml.hpp"
#include <limits> // Add this at the top of the file with other includes
#include <numeric>

// Include OpenMP conditionally
#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
#include <omp.h>
#endif

// Add this forward declaration for the InitializeTestWithChipFunctions function
namespace design {
    void InitializeTestWithChipFunctions(Test& test);
    void InitializeTestWithChipFunctions(Test& test, const std::shared_ptr<Chip>& chip);
}

namespace design {

// Template implementation for FindProcess
template<typename T>
std::shared_ptr<T> Chip::FindProcess(const String& process_name, const std::vector<T>& process_list) const {
    std::shared_ptr<T> process = nullptr;
    for (const auto& p : process_list) {
        if (p.GetName() == process_name) {
            process = std::make_shared<T>(p);
            break;
        }
    }
    if (process == nullptr) {
        std::cout << "Error: Process not found." << std::endl;
    }
    return process;
}

// Explicit template instantiations
template std::shared_ptr<WaferProcess> Chip::FindProcess<WaferProcess>(const String&, const std::vector<WaferProcess>&) const;
template std::shared_ptr<Assembly> Chip::FindProcess<Assembly>(const String&, const std::vector<Assembly>&) const;
template std::shared_ptr<Test> Chip::FindProcess<Test>(const String&, const std::vector<Test>&) const;
template std::shared_ptr<Layer> Chip::FindProcess<Layer>(const String&, const std::vector<Layer>&) const;

// Implementation of FindWaferProcess
std::shared_ptr<WaferProcess> Chip::FindWaferProcess(const String& wafer_process_name, const std::vector<WaferProcess>& wafer_process_list) const {
    std::shared_ptr<WaferProcess> wafer_process = FindProcess<WaferProcess>(wafer_process_name, wafer_process_list);
    if (wafer_process == nullptr) {
        std::cout << "Error: Wafer Process " << wafer_process_name << " not found." << std::endl;
        std::cout << "Exiting" << std::endl;
        exit(1);
    }
    return wafer_process;
}

// Implementation of FindAssemblyProcess
std::shared_ptr<Assembly> Chip::FindAssemblyProcess(const String& assembly_process_name, const std::vector<Assembly>& assembly_process_list) const {    
    std::shared_ptr<Assembly> assembly_process = FindProcess<Assembly>(assembly_process_name, assembly_process_list);
    if (assembly_process == nullptr) {
        std::cout << "Error: Assembly Process " << assembly_process_name << " not found." << std::endl;
        std::cout << "Exiting" << std::endl;
        exit(1);
    }
    return assembly_process;
}

// Implementation of FindTestProcess
std::shared_ptr<Test> Chip::FindTestProcess(const String& test_process_name, const std::vector<Test>& test_process_list) const {
    std::shared_ptr<Test> test_process = FindProcess<Test>(test_process_name, test_process_list);
    if (test_process == nullptr) {
        std::cout << "Error: Test Process " << test_process_name << " not found." << std::endl;
        std::cout << "Exiting" << std::endl;
        exit(1);
    }
    return test_process;
}

// Implementation of BuildStackup
std::vector<Layer> Chip::BuildStackup(const String& stackup_string, const std::vector<Layer>& layers) const {
    std::vector<Layer> stackup;
    std::vector<String> stackup_names;
    
    // Split the stackup string at the commas
    std::istringstream ss(stackup_string);
    String layer_token;
    while (std::getline(ss, layer_token, ',')) {
        size_t colon_pos = layer_token.find(':');
        if (colon_pos != String::npos) {
            String count_str = layer_token.substr(0, colon_pos);
            String layer_name = layer_token.substr(colon_pos + 1);
            
            // Convert count to integer
            IntType count;
            try {
                count = std::stoi(count_str);
            } catch (const std::exception& e) {
                std::cerr << "Error converting count_str '" << count_str << "' to integer: " << e.what() << std::endl;
                count = 0; // Provide a default value or handle error appropriately
            }
            
            if (count >= 0) {
                for (IntType i = 0; i < count; ++i) {
                    stackup_names.push_back(layer_name);
                }
            } else {
                std::cout << "Error: Number of layers " << count_str << " not valid for " << layer_name << "." << std::endl;
                exit(1);
            }
        }
    }
    
    IntType n_layers = static_cast<IntType>(stackup_names.size());
    
    for (const auto& layer_name : stackup_names) {
        std::shared_ptr<Layer> l = FindProcess<Layer>(layer_name, layers);
        
        if (l != nullptr) {
            stackup.push_back(*l);
        } else {
            std::cout << "Error: Layer " << layer_name << " not found." << std::endl;
            std::cout << "Exiting" << std::endl;
            exit(1);
        }
    }
    
    if (static_cast<IntType>(stackup.size()) != n_layers) {
        std::cout << "Error: Stackup number of layers does not match definition, make sure all selected layers are included in the layer definition." << std::endl;
        exit(1);
    }
    
    return stackup;
}

// Constructor implementation
Chip::Chip(
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
    BoolType static_value)
    : parent_chip_(parent_chip),
      static_(false) // Initialize to false first, will be set correctly at end of constructor
{
    // Check if critical parameters are defined, just like in Python
    if (wafer_process_list == nullptr) {
        std::cout << "wafer_process_list is None" << std::endl;
        std::exit(1);
    }
    else if (assembly_process_list == nullptr) {
        std::cout << "assembly_process_list is None" << std::endl;
        std::exit(1);
    }
    else if (test_process_list == nullptr) {
        std::cout << "test_process_list is None" << std::endl;
        std::exit(1);
    }
    else if (layers == nullptr) {
        std::cout << "layers is None" << std::endl;
        std::exit(1);
    }
    else if (ios == nullptr) {
        std::cout << "ios is None" << std::endl;
        std::exit(1);
    }
    else if (adjacency_matrix_definitions == nullptr) {
        std::cout << "adjacency_matrix_definitions is None" << std::endl;
        std::exit(1);
    }
    else if (average_bandwidth_utilization == nullptr) {
        std::cout << "average_bandwidth_utilization is None" << std::endl;
        std::exit(1);
    }
    else if (block_names == nullptr) {
        std::cout << "block_names is None" << std::endl;
        std::exit(1);
    }

    // Initialize root node
    pugi::xml_node root;
    
    // Handle XML parsing similar to Python's ElementTree
    if (!filename.empty() && etree == nullptr) {
        // Parse XML file using PugiXML
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filename.c_str());
        
        if (!result) {
            std::cout << "Error: Failed to parse XML file: " << filename << std::endl;
            std::cout << "Error description: " << result.description() << std::endl;
            std::exit(1);
        }
        
        root = doc.document_element();
    }
    else if (filename.empty() && etree != nullptr) {
        // Use provided XML node
        root = *static_cast<pugi::xml_node*>(etree);
    }
    else if ((filename.empty() || filename == DEFAULT_STRING_VALUE) && etree == nullptr) {
        std::cout << "Error: Invalid chip definition. The filename and etree are both None or empty. Exiting..." << std::endl;
        std::exit(1);
    }
    else {
        std::cout << "Error: Invalid Chip definition. Filename and etree are both defined leading to possible ambiguity. Exiting..." << std::endl;
        std::exit(1);
    }
    
    // Set parent chip
    parent_chip_ = parent_chip;
    
    // Get attributes from XML
    pugi::xml_attribute attr;
    
    // Find processes based on attributes
    attr = root.attribute("wafer_process");
    if (attr) {
        wafer_process_ = FindWaferProcess(attr.value(), *wafer_process_list);
    }
    
    attr = root.attribute("assembly_process");
    if (attr) {
        assembly_process_ = FindAssemblyProcess(attr.value(), *assembly_process_list);
    }
    
    attr = root.attribute("test_process");
    if (attr) {
        test_process_ = FindTestProcess(attr.value(), *test_process_list);
    }
    
    attr = root.attribute("stackup");
    if (attr) {
        stackup_ = BuildStackup(attr.value(), *layers);
    }

    // Set Black-Box Parameters
    attr = root.attribute("bb_area");
    if (attr && attr.value()[0] != '\0') {
        bb_area_ = std::stof(attr.value());
    } else {
        bb_area_ = DEFAULT_FLOAT_VALUE;
    }

    attr = root.attribute("bb_cost");
    if (attr && attr.value()[0] != '\0') {
        bb_cost_ = std::stof(attr.value());
    } else {
        bb_cost_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("bb_quality");
    if (attr && attr.value()[0] != '\0') {
        bb_quality_ = std::stof(attr.value());
    } else {
        bb_quality_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("bb_power");
    if (attr && attr.value()[0] != '\0') {
        bb_power_ = std::stof(attr.value());
    } else {
        bb_power_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("aspect_ratio");
    if (attr && attr.value()[0] != '\0') {
        aspect_ratio_ = std::stof(attr.value());
    } else {
        aspect_ratio_ = 1.0; // Default value in Python
    }
    
    attr = root.attribute("x_location");
    if (attr && attr.value()[0] != '\0') {
        x_location_ = std::stof(attr.value());
    } else {
        x_location_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("y_location");
    if (attr && attr.value()[0] != '\0') {
        y_location_ = std::stof(attr.value());
    } else {
        y_location_ = DEFAULT_FLOAT_VALUE;
    }
    
    // Chip name should match the name in the netlist file
    attr = root.attribute("name");
    if (attr) {
        name_ = attr.value();
    } else {
        name_ = DEFAULT_STRING_VALUE;
    }
    
    // Core area
    attr = root.attribute("core_area");
    if (attr) {
        core_area_ = std::stof(attr.value());
    } else {
        core_area_ = DEFAULT_FLOAT_VALUE;
    }
    
    // NRE Design Cost parameters
    attr = root.attribute("fraction_memory");
    if (attr) {
        fraction_memory_ = std::stof(attr.value());
    } else {
        fraction_memory_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("fraction_logic");
    if (attr) {
        fraction_logic_ = std::stof(attr.value());
    } else {
        fraction_logic_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("fraction_analog");
    if (attr) {
        fraction_analog_ = std::stof(attr.value());
    } else {
        fraction_analog_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("gate_flop_ratio");
    if (attr) {
        gate_flop_ratio_ = std::stof(attr.value());
    } else {
        gate_flop_ratio_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("reticle_share");
    if (attr) {
        reticle_share_ = std::stof(attr.value());
    } else {
        reticle_share_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("quantity");
    if (attr) {
        quantity_ = std::stoll(attr.value());
    } else {
        quantity_ = DEFAULT_INT_VALUE;
    }
    
    // Buried attribute
    attr = root.attribute("buried");
    if (attr) {
        String buried_str = attr.value();
        std::transform(buried_str.begin(), buried_str.end(), buried_str.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        buried_ = (buried_str == "true");
    } else {
        buried_ = false;
    }
    
    // Copy the adjacency matrix if provided
    if (adjacency_matrix_definitions) {
        global_adjacency_matrix_ = *adjacency_matrix_definitions;
    }
    
    // Copy average bandwidth utilization if provided
    if (average_bandwidth_utilization) {
        average_bandwidth_utilization_ = *average_bandwidth_utilization;
    } else {
        // Initialize with an empty structure
        average_bandwidth_utilization_ = std::map<String, std::vector<std::vector<FloatType>>>();
    }
    
    if (block_names) {
        block_names_ = *block_names;
    }
    
    if (ios) {
        std::vector<std::shared_ptr<IO>> tmp;
        for (const auto &io_obj : *ios) {
            tmp.push_back(std::make_shared<IO>(io_obj));
        }
        io_list_ = tmp;
    }
    
    // Power and Core Voltage
    attr = root.attribute("power");
    if (attr) {
        power_ = std::stof(attr.value());
    } else {
        power_ = DEFAULT_FLOAT_VALUE;
    }
    
    attr = root.attribute("core_voltage");
    if (attr) {
        core_voltage_ = std::stof(attr.value());
    } else {
        core_voltage_ = DEFAULT_FLOAT_VALUE;
    }

    // Check if the chip is fully defined
    if (name_.empty() || name_ == DEFAULT_STRING_VALUE) {
        std::cout << "Error: Chip name is \"\". Exiting..." << std::endl;
        std::exit(1);
    } else if (wafer_process_ == nullptr) {
        std::cout << "wafer_process is None" << std::endl;
        std::exit(1);
    } else if (assembly_process_ == nullptr) {
        std::cout << "assembly_process is None" << std::endl;
        std::exit(1);
    } else if (test_process_ == nullptr) {
        std::cout << "test_process is None" << std::endl;
        std::exit(1);
    } else if (stackup_.empty()) {
        std::cout << "stackup is None" << std::endl;
        std::exit(1);
    }

    // Compute power for all chips stacked on top of the current chip
    SetStackPower(ComputeStackPower());
    
    // Compute IO power  
    FloatType io_power = GetSignalPower(GetChipList());
    IntType io_return = SetIoPower(io_power);

    // Compute total power
    if (bb_power_ == DEFAULT_FLOAT_VALUE) {
        SetTotalPower(power_ + GetIoPower() + GetStackPower());
    } else {
        // bb_power overrides all power specific to the chip
        SetTotalPower(bb_power_ + GetStackPower());
    }
    
    // Compute NRE design cost
    SetNreDesignCost();

    // Set the chip area
    SetArea();

    // Compute yields and costs
    SetSelfTrueYield(ComputeLayerAwareYield());
    // Set static value last
    static_ = static_value;
}

// Add a new Initialize method to be called after constructing the chip
void Chip::Initialize() {
    // Now that the shared_ptr exists, we can call methods that use shared_from_this()
    if (test_process_) {
        // No longer need to initialize the Test with Chip functions
        SetSelfTestYield(test_process_->ComputeSelfTestYield(GetSelfTrueYield(), GetName()));
        
        if (bb_quality_ == DEFAULT_FLOAT_VALUE) {
            SetSelfQuality(test_process_->ComputeSelfQuality(GetSelfTrueYield(), GetSelfTestYield()));
        } else {
            SetSelfQuality(bb_quality_);
        }
    }
    
    SetChipTrueYield(ComputeChipYield());

    if (test_process_) {
        SetChipTestYield(test_process_->ComputeAssemblyTestYield(GetChipTrueYield()));
        SetQuality(test_process_->ComputeAssemblyQuality(GetChipTrueYield(), GetChipTestYield()));
    }

    SetSelfCost(ComputeSelfCost());

    SetCost(ComputeCost());
}

// Getter and setter implementations
String Chip::GetName() const {
    return name_;
}

IntType Chip::SetName(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value.empty()) { // In C++, we check if string is empty instead of type check
            std::cout << "Error: Chip name must be a string." << std::endl;
            return 1;
        } else {
            name_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetCoreArea() const {
    return core_area_;
}

IntType Chip::SetCoreArea(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, we can't check if it's a string type because of static typing
        // But we can check if it's negative
        if (value < 0) {
            std::cout << "Error: Core area must be nonnegative." << std::endl;
            return 1;
        } else {
            core_area_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetAspectRatio() const {
    return aspect_ratio_;
}

IntType Chip::SetAspectRatio(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, we can't check if it's a string type because of static typing
        // But we can check if it's negative
        if (value < 0) {
            std::cout << "Error: Aspect ratio must be nonnegative." << std::endl;
            return 1;
        } else {
            aspect_ratio_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetXLocation() const {
    return x_location_;
}

IntType Chip::SetXLocation(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            x_location_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: X location must be nonnegative." << std::endl;
            return 1;
        } else {
            x_location_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetYLocation() const {
    return y_location_;
}

IntType Chip::SetYLocation(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            y_location_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: Y location must be nonnegative." << std::endl;
            return 1;
        } else {
            y_location_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetBbArea() const {
    return bb_area_;
}

IntType Chip::SetBbArea(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_area_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB area must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_area_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetBbCost() const {
    return bb_cost_;
}

IntType Chip::SetBbCost(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_cost_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } else if (value < 0) {
            std::cout << "Error: BB cost must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_cost_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetBbQuality() const {
    return bb_quality_;
}

IntType Chip::SetBbQuality(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In Python, None or empty string would set the value to None
        // In C++, we use DEFAULT_FLOAT_VALUE instead of None
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_quality_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } 
        // In C++, type checking handled by static typing (FloatType parameter)
        // whereas Python had: elif type(value) == str
        else if (value < 0 || value > 1) {
            std::cout << "Error: BB quality must be between 0 and 1." << std::endl;
            return 1;
        } else {
            bb_quality_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetBbPower() const {
    return bb_power_;
}

IntType Chip::SetBbPower(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In Python, None or empty string would set the value to None
        // In C++, we use DEFAULT_FLOAT_VALUE instead of None
        if (value == DEFAULT_FLOAT_VALUE) {
            bb_power_ = DEFAULT_FLOAT_VALUE;
            return 0;
        } 
        // In C++, type checking handled by static typing (FloatType parameter)
        // whereas Python had: elif type(value) == str
        else if (value < 0) {
            std::cout << "Error: BB power must be nonnegative." << std::endl;
            return 1;
        } else {
            bb_power_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetFractionMemory() const {
    return fraction_memory_;
}

IntType Chip::SetFractionMemory(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, type checking is handled by static typing (FloatType parameter)
        // whereas Python had an explicit check: if type(value) == str
        if (value < 0 || value > 1) {
            std::cout << "Error: Fraction memory must be between 0 and 1." << std::endl;
            return 1;
        } else {
            fraction_memory_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetFractionLogic() const {
    return fraction_logic_;
}

IntType Chip::SetFractionLogic(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, type checking is handled by static typing (FloatType parameter)
        // whereas Python had an explicit check: if type(value) == str
        if (value < 0 || value > 1) {
            std::cout << "Error: Fraction logic must be between 0 and 1." << std::endl;
            return 1;
        } else {
            fraction_logic_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetFractionAnalog() const {
    return fraction_analog_;
}

IntType Chip::SetFractionAnalog(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, type checking is handled by static typing (FloatType parameter)
        // whereas Python had an explicit check: if type(value) == str
        if (value < 0 || value > 1) {
            std::cout << "Error: Fraction analog must be between 0 and 1." << std::endl;
            return 1;
        } else {
            fraction_analog_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetGateFlopRatio() const {
    return gate_flop_ratio_;
}

IntType Chip::SetGateFlopRatio(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, type checking is handled by static typing (FloatType parameter)
        // whereas Python had an explicit check: if type(value) == str
        if (value < 0) {
            std::cout << "Error: Gate flop ratio must be nonnegative." << std::endl;
            return 1;
        } else {
            gate_flop_ratio_ = value;
            return 0;
        }
    }
}

FloatType Chip::GetReticleShare() const {
    return reticle_share_;
}

IntType Chip::SetReticleShare(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // In C++, type checking is handled by static typing (FloatType parameter)
        // whereas Python had an explicit check: if type(value) == str
        if (value < 0) {
            std::cout << "Error: Reticle share must be nonnegative." << std::endl;
            return 1;
        } else {
            reticle_share_ = value;
            return 0;
        }
    }
}

BoolType Chip::GetBuried() const {
    return buried_;
}

IntType Chip::SetBuried(const String& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        if (value.empty()) {
            std::cout << "Error: Buried must be a string with value \"True\" or \"true\"." << std::endl;
            return 1;
        } else {
            String lowerValue = value;
            std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (lowerValue == "true") {
                buried_ = true;
            } else {
                buried_ = false;
            }
            return 0;
        }
    }
}

std::vector<std::shared_ptr<Chip>> Chip::GetChips() const {
    return chips_;
}

IntType Chip::SetChips(const std::vector<std::shared_ptr<Chip>>& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        chips_ = value;
        return 0;
    }
}

std::shared_ptr<Assembly> Chip::GetAssemblyProcess() const {
    return assembly_process_;
}

IntType Chip::SetAssemblyProcess(const std::shared_ptr<Assembly>& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        assembly_process_ = value;
        return 0;
    }
}

std::shared_ptr<Test> Chip::GetTestProcess() const {
    return test_process_;
}

IntType Chip::SetTestProcess(const std::shared_ptr<Test>& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        test_process_ = value;
        return 0;
    }
}

std::vector<Layer> Chip::GetStackup() const {
    return stackup_;
}

IntType Chip::SetStackup(const std::vector<Layer>& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        stackup_ = value;
        return 0;
    }
}

std::shared_ptr<WaferProcess> Chip::GetWaferProcess() const {
    return wafer_process_;
}

IntType Chip::SetWaferProcess(const std::shared_ptr<WaferProcess>& value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        wafer_process_ = value;
        return 0;
    }
}

FloatType Chip::GetCoreVoltage() const {
    return core_voltage_;
}

IntType Chip::SetCoreVoltage(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        core_voltage_ = value;
        return 0;
    }
}

FloatType Chip::GetPower() const {
    return power_;
}

IntType Chip::SetPower(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        power_ = value;
        return 0;
    }
}

long long Chip::GetQuantity() const {
    return quantity_;
}

IntType Chip::SetQuantity(long long value) {
    if (static_) {
        std::cout << "Error: Cannot change the static chip." << std::endl;
        return 1;
    } else {
        if (value < 0) {
            std::cout << "Error: Quantity must be nonnegative." << std::endl;
            return 1;
        } else {
            quantity_ = value;
            return 0;
        }
    }
}

BoolType Chip::GetStatic() const {
    return static_;
}

IntType Chip::SetStatic() {
    static_ = true;
    return 0;
}

// Method required by TestChipBinding
FloatType Chip::GetSelfGatesPerMm2() const {
    FloatType self_gates_per_mm2 = 0.0;
    for (const auto& layer : stackup_) {
        self_gates_per_mm2 += layer.GetGatesPerMm2();
    }
    return self_gates_per_mm2;
}

// Method required by TestChipBinding
FloatType Chip::GetAssemblyGatesPerMm2() const {
    FloatType total_core_area = GetAssemblyCoreArea();
    FloatType assembly_gates_per_mm2 = GetSelfGatesPerMm2();
    
    for (const auto& chip : chips_) {
        if (chip != nullptr) {
            total_core_area += chip->GetAssemblyCoreArea();
            assembly_gates_per_mm2 += chip->GetAssemblyGatesPerMm2();
        }
    }
    
    if (total_core_area == 0) {
        assembly_gates_per_mm2 = 0.0;
    } else {
        assembly_gates_per_mm2 = assembly_gates_per_mm2 / total_core_area;
    }
    
    return assembly_gates_per_mm2;
}

// Operator overload for stream output
std::ostream& operator<<(std::ostream& os, const Chip& chip) {
    os << "Chip " << chip.name_ << std::endl;
    os << "  core_area: " << chip.core_area_ << std::endl;
    os << "  aspect_ratio: " << chip.aspect_ratio_ << std::endl;
    os << "  x_location: " << chip.x_location_ << std::endl;
    os << "  y_location: " << chip.y_location_ << std::endl;
    os << "  bb_area: " << chip.bb_area_ << std::endl;
    os << "  bb_cost: " << chip.bb_cost_ << std::endl;
    os << "  bb_quality: " << chip.bb_quality_ << std::endl;
    os << "  bb_power: " << chip.bb_power_ << std::endl;
    os << "  fraction_memory: " << chip.fraction_memory_ << std::endl;
    os << "  fraction_logic: " << chip.fraction_logic_ << std::endl;
    os << "  fraction_analog: " << chip.fraction_analog_ << std::endl;
    os << "  gate_flop_ratio: " << chip.gate_flop_ratio_ << std::endl;
    os << "  reticle_share: " << chip.reticle_share_ << std::endl;
    os << "  buried: " << (chip.buried_ ? "True" : "False") << std::endl;
    os << "  number of child chips: " << chip.chips_.size() << std::endl;
    os << "  core_voltage: " << chip.core_voltage_ << std::endl;
    os << "  power: " << chip.power_ << std::endl;
    os << "  quantity: " << chip.quantity_ << std::endl;
    os << "  static: " << (chip.static_ ? "True" : "False");
    return os;
}

// Computation method implementations
FloatType Chip::GetAssemblyCoreArea() const {
    FloatType assembly_core_area = core_area_;
    for (const auto& chip : chips_) {
        if (chip != nullptr) {
            assembly_core_area += chip->GetAssemblyCoreArea();
        }
    }
    return assembly_core_area;
}

IntType Chip::GetChipsLen() const {
    return static_cast<IntType>(chips_.size());
}

std::shared_ptr<Chip> Chip::GetParentChip() const {
    return parent_chip_;
}

FloatType Chip::GetWaferDiameter() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    return wafer_process_->GetWaferDiameter();
}

FloatType Chip::GetStackPower() const {
    return stack_power_;
}

IntType Chip::SetStackPower(FloatType value) {
    stack_power_ = ComputeStackPower();
    return 0;
}

FloatType Chip::GetIoPower() const {
    return io_power_;
}

IntType Chip::SetIoPower(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        io_power_ = value;
        return 0;
    }
}

FloatType Chip::GetTotalPower() const {
    return total_power_;
}

IntType Chip::SetTotalPower(FloatType value) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        total_power_ = value;
        return 0;
    }
}

FloatType Chip::ComputeNreFrontEndCost() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    FloatType front_end_cost = core_area_ * (
        wafer_process_->GetNreFrontEndCostPerMm2Memory() * fraction_memory_ +
        wafer_process_->GetNreFrontEndCostPerMm2Logic() * fraction_logic_ +
        wafer_process_->GetNreFrontEndCostPerMm2Analog() * fraction_analog_
    );
    
    return front_end_cost;
}

FloatType Chip::ComputeNreBackEndCost() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    FloatType back_end_cost = core_area_ * (
        wafer_process_->GetNreBackEndCostPerMm2Memory() * fraction_memory_ +
        wafer_process_->GetNreBackEndCostPerMm2Logic() * fraction_logic_ +
        wafer_process_->GetNreBackEndCostPerMm2Analog() * fraction_analog_
    );
    
    return back_end_cost;
}

FloatType Chip::ComputeNreCost() const {
    // Calculate base NRE cost
    FloatType nre_cost = (GetNreDesignCost() + GetMaskCost());
    
    // Add ATPG cost from test process if available
    if (test_process_) {
        std::shared_ptr<Chip> non_const_this = std::const_pointer_cast<Chip>(shared_from_this());
        float temp_atgp_cost = test_process_->GetAtpgCost(
            GetCoreArea(), 
            GetCoreArea(), // Use core area for assembly core area
            GetSelfGatesPerMm2(), 
            GetSelfGatesPerMm2(), // Use self gates per mm2 for assembly gates per mm2
            GetGateFlopRatio());
        nre_cost += temp_atgp_cost;
    }
    
    // Divide by quantity
    if (quantity_ > 0) {
        nre_cost /= quantity_;
    }

    // Add NRE costs from all child chips
    for (const auto& chip : chips_) {
        if (chip) {
            nre_cost += chip->ComputeNreCost();
        }
    }

    return nre_cost;
}

FloatType Chip::ComputeNreDesignCost() const {
    FloatType nre_design_cost = ComputeNreFrontEndCost() + ComputeNreBackEndCost();
    return nre_design_cost;
}

FloatType Chip::GetNreDesignCost() const {
    return nre_design_cost_;
}

IntType Chip::SetNreDesignCost() {
    nre_design_cost_ = ComputeNreDesignCost();
    return 0;
}

FloatType Chip::ComputeLayerAwareYield() const {
    FloatType layer_yield = 1.0;
    
    for (const auto& layer : stackup_) {
        // Use the Layer's layer_yield method with the chip's area
        layer_yield *= layer.LayerYield(core_area_ + GetIoArea());
    }
    
    return layer_yield;
}

FloatType Chip::GetSignalPower(const std::vector<String>& internal_block_list) const {
    // print the internal block list 
    FloatType signal_power = 0.0;
    IntType block_index = -1;
    std::vector<IntType> internal_block_list_indices;
    
    // Find block index and internal block list indices
    
    for (IntType i = 0; i < static_cast<IntType>(block_names_.size()); i++) {
        if (block_names_[i] == name_) {
            block_index = i;
        }
        
        // Check if block_names_[i] is in internal_block_list
        if (std::find(internal_block_list.begin(), internal_block_list.end(), block_names_[i]) != internal_block_list.end()) {
            internal_block_list_indices.push_back(i);
        }
    }
    
    // If chip name not found in block list, return zeros
    if (block_index == -1) {
        return 0.0;
    }

    // Process each IO type in the adjacency matrix
    for (const auto& matrix_entry : global_adjacency_matrix_) {
        const String& io_type = matrix_entry.first;
        const auto& connection_matrix = matrix_entry.second;
        
        // Find the IO with matching type
        std::shared_ptr<design::IO> matching_io;
        for (const auto& io : io_list_) {
            if (io->GetType() == io_type) {
                matching_io = io;
                break;
            }
        }
        
        if (!matching_io) {
            continue; // Skip if no matching IO found
        }

        // Set bidirectional factor
        FloatType bidirectional_factor = matching_io->GetBidirectional() ? 0.5 : 1.0;
        auto bw_it = average_bandwidth_utilization_.find(io_type);
        if (bw_it == average_bandwidth_utilization_.end()) {
            continue;
        }

        // Process connections for this block
        for (IntType j = 0; j < static_cast<IntType>(block_names_.size()); j++) {
            // Skip if j is in internal_block_list_indices
            if (std::find(internal_block_list_indices.begin(), internal_block_list_indices.end(), j) != internal_block_list_indices.end()) {
                continue;
            }
            
            // Get connection value from block_index to j and j to block_index
            FloatType outgoing_value = 0.0;
            FloatType incoming_value = 0.0;

            // Use 2D vector indexing to get connection values
            if (block_index < static_cast<IntType>(connection_matrix.size()) && 
                j < static_cast<IntType>(connection_matrix[block_index].size())) {
                outgoing_value = static_cast<FloatType>(connection_matrix[block_index][j]) * bw_it->second[block_index][j];
            }
            
            if (j < static_cast<IntType>(connection_matrix.size()) && 
                block_index < static_cast<IntType>(connection_matrix[j].size())) {
                incoming_value = static_cast<FloatType>(connection_matrix[j][block_index]) * bw_it->second[j][block_index];
            }
            signal_power += (outgoing_value + incoming_value); 
        }
        signal_power *= matching_io->GetBandwidth() * matching_io->GetEnergyPerBit() * bidirectional_factor;
    }
    return signal_power;
}

FloatType Chip::ComputeChipYield() const {
    // Account for the quality of the chip after self-test
    FloatType chip_true_yield = GetSelfQuality();
    
    // Account for quality of component chiplets
    FloatType quality_yield = QualityYield();
    
    // Account for assembly yield
    FloatType assembly_yield = 1.0;
    if (assembly_process_) {
        assembly_yield = assembly_process_->AssemblyYield(
            GetChipsLen(),
            GetChipsSignalCount(),
            GetStackedDieArea()
        );
    }
    
    // Account for wafer process yield
    FloatType wafer_process_yield = 1.0;
    if (wafer_process_) {
        wafer_process_yield = wafer_process_->GetWaferProcessYield();
    }
    
    // Multiply the yields
    chip_true_yield *= quality_yield * assembly_yield * wafer_process_yield;
    
    return chip_true_yield;
}

FloatType Chip::ComputeSelfCost() const {
    FloatType cost = 0.0;
    
    // Check if bb_cost is set
    if (bb_cost_ != DEFAULT_FLOAT_VALUE) {
        cost = bb_cost_;
    } else {
        // Add layer-aware cost
        FloatType self_layer_cost = GetLayerAwareCost();
        cost += self_layer_cost;
        
        // Add test cost if test process is available
        if (test_process_) {
            // The ComputeSelfTestCost method expects a non-const Chip
            std::shared_ptr<Chip> non_const_this = std::const_pointer_cast<Chip>(shared_from_this());
            FloatType self_test_cost = test_process_->ComputeSelfTestCost(
                GetCoreArea(), 
                GetSelfGatesPerMm2(), 
                GetGateFlopRatio());
            cost += self_test_cost;
        }
        
        // Adjust for self test yield
        FloatType self_test_yield = GetSelfTestYield();
        if (self_test_yield > 0) {
            cost = cost / self_test_yield;
        } else {
            std::cout << "Error: Self test yield is zero or negative. Exiting..." << std::endl;
            std::exit(1);
        }
    }
    
    return cost;
}

FloatType Chip::ComputeCost() const {
    // Otherwise compute cost based on yield and other factors
    FloatType cost = GetSelfCost();
    FloatType stack_cost = 0.0;
    // Add costs of all component chips
    for (const auto& chip : chips_) {
        if (chip != nullptr) {
            stack_cost += chip->ComputeCost();
            cost += chip->ComputeCost();
        }
    }

    FloatType assembly_cost = 0.0;
    FloatType test_cost = 0.0;
    
    // Add assembly cost if we have an assembly process
    if (assembly_process_) {
        assembly_cost = assembly_process_->AssemblyCost(
            GetChipsLen(),
            GetStackedDieArea()
        );
    }

    cost += assembly_cost;
    
    // Add test cost if we have a test process
    if (test_process_) {
        // The ComputeAssemblyTestCost method expects a non-const Chip
        std::shared_ptr<Chip> non_const_this = std::const_pointer_cast<Chip>(shared_from_this());
        FloatType assembly_test_cost = test_process_->ComputeAssemblyTestCost(
            GetAssemblyCoreArea(), 
            GetAssemblyGatesPerMm2());
        test_cost += assembly_test_cost;
    }

    cost += test_cost;
    
    FloatType denominator_cost = GetChipTestYield();
    cost /= denominator_cost;

    return cost;
}

FloatType Chip::ComputeDieCost() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    FloatType die_area = core_area_;
    if (die_area <= 0) {
        return 0.0;
    }
    
    FloatType x_dim, y_dim;
    if (aspect_ratio_ > 0) {
        x_dim = std::sqrt(die_area * aspect_ratio_);
        y_dim = die_area / x_dim;
    } else {
        x_dim = std::sqrt(die_area);
        y_dim = x_dim;
    }
    
    FloatType wafer_diameter = wafer_process_->GetWaferDiameter();
    FloatType edge_exclusion = wafer_process_->GetEdgeExclusion();
    FloatType dicing_distance = wafer_process_->GetDicingDistance();
    BoolType wafer_fill_grid = wafer_process_->GetWaferFillGrid();
    
    
    FloatType usable_wafer_diameter = wafer_diameter - (2 * edge_exclusion);
    
    Layer temp_layer;
    IntType dies_per_wafer = temp_layer.ComputeDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, dicing_distance, wafer_fill_grid);
    
    if (dies_per_wafer <= 0) {
        return 0.0;
    }
    
    FloatType wafer_cost = 1000.0;
    FloatType wafer_process_yield = wafer_process_->GetWaferProcessYield();
    
    FloatType die_cost = wafer_cost / (dies_per_wafer * wafer_process_yield);
    
    return die_cost;
}

FloatType Chip::ComputeSelfPerfectYieldCost() const {
    FloatType cost = 0.0;
    
    // The bb_cost parameter will override the self cost computation
    if (bb_cost_ != DEFAULT_FLOAT_VALUE) {
        cost = bb_cost_;
    } else {
        // Add cost of this chip
        FloatType self_layer_cost = GetLayerAwareCost();
        cost += self_layer_cost;
        
        // Add test cost
        if (test_process_) {
            // The ComputeSelfTestCost method expects a non-const Chip
            std::shared_ptr<Chip> non_const_this = std::const_pointer_cast<Chip>(shared_from_this());
            FloatType self_test_cost = test_process_->ComputeSelfTestCost(
                GetCoreArea(), 
                GetSelfGatesPerMm2(), 
                GetGateFlopRatio());
            cost += self_test_cost;
        }
    }
    
    return cost;
}

FloatType Chip::ComputePerfectYieldCost() const {
    FloatType self_layer_cost = GetLayerAwareCost();
    FloatType assembly_cost = 0.0;
    FloatType assembly_test_cost = 0.0;
    
    // Add assembly cost if there's an assembly process
    if (assembly_process_) {
        assembly_cost = assembly_process_->AssemblyCost(
            GetChipsLen(),
            GetStackedDieArea()
        );
    }
    
    // Add assembly test cost if there's a test process
    if (test_process_) {
        // The ComputeAssemblyTestCost method expects a non-const Chip
        std::shared_ptr<Chip> non_const_this = std::const_pointer_cast<Chip>(shared_from_this());
        assembly_test_cost = test_process_->ComputeAssemblyTestCost(
            GetAssemblyCoreArea(),
            GetAssemblyGatesPerMm2());
    }
    
    // Calculate total cost
    FloatType total_cost = self_layer_cost + assembly_cost + assembly_test_cost;
    
    // Add costs for child chips
    for (const auto& chip : chips_) {
        if (chip != nullptr) {
            total_cost += chip->ComputePerfectYieldCost();
        }
    }
    
    return total_cost;
}

FloatType Chip::ComputeScrapCost() const {
    return ComputeCost() - ComputePerfectYieldCost();
}

FloatType Chip::ComputeTotalNonScrapCost() const {
    return ComputePerfectYieldCost() + ComputeNreCost();
}

FloatType Chip::ComputeTotalCost() const {
    FloatType total_cost = ComputeCost() + ComputeNreCost();
    return total_cost;
}

FloatType Chip::ComputeStackPower() const {
    FloatType stack_power = 0.0;

#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
    // Only parallelize if there are enough chips (threshold determined empirically)
    if (chips_.size() > 4) {
        // Using reduction to accumulate power from all threads
        #pragma omp parallel for reduction(+:stack_power) schedule(dynamic)
        for (size_t i = 0; i < chips_.size(); i++) {
            if (chips_[i] != nullptr) {
                stack_power += chips_[i]->GetTotalPower();
            }
        }
    }
    else 
#endif
    {
        // Sequential version
        for (const auto& chip : chips_) {
            if (chip != nullptr) {
                stack_power += chip->GetTotalPower();
            }
        }
    }

    return stack_power;
}

FloatType Chip::GetStackedDieArea() const {
    if (!assembly_process_) {
        return 0.0;
    }
    
    FloatType stacked_die_area = 0.0;
    
    for (const auto& chip : chips_) {
        if (chip && !chip->GetBuried()) {
            FloatType temp_area = ExpandedArea(chip->GetArea(), assembly_process_->GetDieSeparation() / 2.0, chip->GetAspectRatio());
            stacked_die_area += temp_area;
        }
    }

    stacked_die_area = ExpandedArea(stacked_die_area, assembly_process_->GetEdgeExclusion());
    
    return stacked_die_area;
}

FloatType Chip::ExpandedArea(FloatType area, FloatType margin, FloatType aspect_ratio) const {
    if (area <= 0.0) {
        return 0.0;
    }
    
    // Default aspect ratio to 1.0 if it's less than or equal to 0
    if (aspect_ratio <= 0.0) {
        aspect_ratio = 1.0;
    }
    
    // Match Python implementation exactly
    FloatType x = std::sqrt(area * aspect_ratio);
    FloatType y = std::sqrt(area / aspect_ratio);
    FloatType new_area = (x + 2.0 * margin) * (y + 2.0 * margin);
    
    return new_area;
}

FloatType Chip::GetEdgeExclusion() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    return wafer_process_->GetEdgeExclusion();
}

FloatType Chip::GetWaferProcessYield() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    return wafer_process_->GetWaferProcessYield();
}

FloatType Chip::GetReticleX() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    return wafer_process_->GetReticleX();
}

FloatType Chip::GetReticleY() const {
    if (!wafer_process_) {
        return 0.0;
    }
    
    return wafer_process_->GetReticleY();
}

FloatType Chip::GetArea() const {
    return area_;
}

IntType Chip::SetArea() {
    area_ = ComputeArea();
    return 0;
}

FloatType Chip::ComputeArea() const {
    // If bb_area is defined, use it
    if (bb_area_ != DEFAULT_FLOAT_VALUE) {
        return bb_area_;
    }
    
    // Calculate chip IO area
    FloatType chip_io_area = core_area_ + GetIoArea();
    
    // Calculate pad required area
    FloatType pad_required_area = GetPadArea();
    
    // Calculate stacked die area
    FloatType stacked_die_bound_area = GetStackedDieArea();
    
    // Return the maximum of the three areas
    return std::max(std::max(stacked_die_bound_area, pad_required_area), chip_io_area);
}

IntType Chip::SetChipDefinitions(const std::map<String, String>& chip_definitions) {
    if (static_) {
        std::cout << "Error: Cannot change static chip." << std::endl;
        return 1;
    } else {
        // Implementation would need BuildChips method which isn't shown in the context
        // chips_ = BuildChips(chip_definitions);
        return 0;
    }
}

std::vector<String> Chip::GetChipList() const {
    std::vector<String> chip_list;
    // currently flattening the list
    // Add chips from all child chips
    for (const auto& chip : chips_) {
        if (chip) {
            std::vector<String> child_list = chip->GetChipList();
            // Use insert with iterators properly
            chip_list.insert(chip_list.end(), child_list.begin(), child_list.end());
        }
    }
    
    // Add this chip's name
    chip_list.push_back(name_);
    
    return chip_list;
}

FloatType Chip::WaferAreaEff() const {
    // Early return if no wafer process
    if (!wafer_process_) {
        return 0.0;
    }
    
    // Get wafer diameter and edge exclusion from wafer process
    FloatType wafer_diameter = wafer_process_->GetWaferDiameter();
    FloatType edge_exclusion = wafer_process_->GetEdgeExclusion();
    
    // Calculate usable wafer radius
    FloatType usable_wafer_radius = (wafer_diameter / 2.0) - edge_exclusion;
    
    // Calculate and return usable wafer area
    FloatType usable_wafer_area = M_PI * usable_wafer_radius * usable_wafer_radius;
    
    return usable_wafer_area;
}

FloatType Chip::GetLayerAwareCost() const {
    FloatType cost = 0.0;
    
    // Check if we have a wafer process
    if (!wafer_process_) {
        return cost;
    }

#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP) && defined(_OPENMP)
    // Only parallelize if there are enough layers (threshold determined empirically)
    if (stackup_.size() > 4) {
        // Using reduction to accumulate costs from all threads
        #pragma omp parallel for reduction(+:cost) schedule(dynamic)
        for (size_t i = 0; i < stackup_.size(); i++) {
            // Add layer cost for each layer
            cost += stackup_[i].LayerCost(GetArea(), aspect_ratio_, *wafer_process_);
        }
    }
    else 
#endif
    {
        // Sequential version
        // Iterate through all layers in the stackup
        for (const auto& layer : stackup_) {
            // Add layer cost for each layer
            cost += layer.LayerCost(GetArea(), aspect_ratio_, *wafer_process_);
        }
    }
    
    return cost;
}

FloatType Chip::GetIoArea() const {
    // Uses the adjacency matrices to calculate IO area
    FloatType io_area = 0.0;
    
    int block_index = -1;
    for (IntType i = 0; i < static_cast<IntType>(block_names_.size()); i++) {
        if (block_names_[i] == name_) {
            block_index = i;
            break;
        }
    }

    if (block_index == -1) {
        return 0.0;  // Return 0 if chip name not found in block_names_
    }

    // For each IO type in the adjacency matrix
    for (const auto& matrix_entry : global_adjacency_matrix_) {
        const String& io_type = matrix_entry.first;
        const auto& connection_matrix = matrix_entry.second;
        
        // Find the IO with matching type
        std::shared_ptr<design::IO> matching_io;
        for (const auto& io : io_list_) {
            if (io->GetType() == io_type) {
                matching_io = io;
                break;
            }
        }
        
        if (!matching_io) {
            continue;  // Skip if no matching IO found
        }

        // Calculate outgoing and incoming connections for this chip
        FloatType outgoing_sum = 0.0;
        FloatType incoming_sum = 0.0;


        
        // Find the index of this chip in block_names_
        auto chip_it = std::find(block_names_.begin(), block_names_.end(), name_);
        if (chip_it == block_names_.end()) {
            continue;  // Skip if chip name not found in block_names_
        }
        IntType chip_index = static_cast<IntType>(std::distance(block_names_.begin(), chip_it));
        
        // Use 2D vector indexing to count connections
        if (chip_index < static_cast<IntType>(connection_matrix.size())) {
            // Outgoing connections (from this chip to others)
            for (IntType j = 0; j < static_cast<IntType>(connection_matrix[chip_index].size()); j++) {
                outgoing_sum += static_cast<FloatType>(connection_matrix[chip_index][j]);
            }
            
            // Incoming connections (from others to this chip)
            for (IntType i = 0; i < static_cast<IntType>(connection_matrix.size()); i++) {
                if (i < static_cast<IntType>(connection_matrix.size()) && 
                    chip_index < static_cast<IntType>(connection_matrix[i].size())) {
                    incoming_sum += static_cast<FloatType>(connection_matrix[i][chip_index]);
                }
            }
        }
        
        // Add weighted areas to total
        io_area += outgoing_sum * matching_io->GetTxArea() + incoming_sum * matching_io->GetRxArea();
    }
    
    return io_area;
}

IntType Chip::GetPowerPads() const {
    if (!assembly_process_) {
        return 0;
    }
    
    FloatType power = GetTotalPower();
    FloatType power_per_pad = assembly_process_->GetPowerPerPad(core_voltage_);
    
    // Calculate number of power pads needed
    IntType power_pads = static_cast<IntType>(std::ceil(power / power_per_pad));
    
    // Multiply by 2 for ground and power
    power_pads *= 2;
    
    return power_pads;
}

FloatType Chip::GetPadArea() const {
    if (!assembly_process_) {
        return 0.0;
    }
    
    // Get counts of different types of pads
    IntType num_power_pads = GetPowerPads();
    IntType num_test_pads = test_process_ ? test_process_->NumTestIos() : 0;
    
    // Get signal pads count and signal_with_reach_count map exactly as in Python
    std::pair<IntType, std::map<String, IntType>> signal_count_result = GetSignalCount(GetChipList());
    IntType signal_pads = signal_count_result.first;
    std::map<String, IntType> signal_with_reach_count = signal_count_result.second;
    
    IntType num_pads = signal_pads + num_power_pads + num_test_pads;
    
    // Get bonding pitch from the parent chip's assembly process, or from our own if no parent
    FloatType bonding_pitch = 0.0;
    std::shared_ptr<Chip> parent_chip = GetParentChip();
    if (parent_chip && parent_chip->GetAssemblyProcess()) {
        bonding_pitch = parent_chip->GetAssemblyProcess()->GetBondingPitch();
    } else if (assembly_process_) {
        bonding_pitch = assembly_process_->GetBondingPitch();
    } else {
        // Default value if no assembly process is available
        bonding_pitch = 0.1;
    }
    
    FloatType area_per_pad = bonding_pitch * bonding_pitch;
    
    // Create and sort the reaches
    std::vector<FloatType> reaches;
    for (const auto& pair : signal_with_reach_count) {
        reaches.push_back(std::stof(pair.first));
    }
    std::sort(reaches.begin(), reaches.end());
    
    FloatType current_x = 0.0;
    FloatType current_y = 0.0;
    IntType current_count = 0;
    
    for (const auto& reach : reaches) {
        // Calculate reach with separation
        FloatType reach_with_separation = 0.0;
        if (parent_chip && parent_chip->GetAssemblyProcess()) {
            reach_with_separation = reach - parent_chip->GetAssemblyProcess()->GetDieSeparation();
        } else if (assembly_process_) {
            reach_with_separation = reach - assembly_process_->GetDieSeparation();
        } else {
            reach_with_separation = reach - 0.1; // Default separation
        }
        
        if (reach_with_separation < 0) {
            std::cout << "Error: Reach is smaller than chip separation. Exiting..." << std::endl;
            exit(1);
        }
        
        // Get signal count for this reach
        String reach_str = std::to_string(reach);
        current_count += signal_with_reach_count[reach_str];
        
        // Find the minimum boundary that would contain all the pads with the current reach
        FloatType required_area = current_count * area_per_pad;
        FloatType usable_area = 0.0;
        
        if (reach_with_separation < current_x && reach_with_separation < current_y) {
            usable_area = reach_with_separation * (current_x + current_y) - reach_with_separation * reach_with_separation;
        } else {
            usable_area = current_x * current_y;
        }
        
        if (usable_area <= required_area) {
            // Calculate required dimensions
            FloatType required_x = std::sqrt(required_area * aspect_ratio_);
            FloatType required_y = std::sqrt(required_area / aspect_ratio_);
            
            FloatType new_req_x = 0.0;
            FloatType new_req_y = 0.0;
            
            if (required_x > reach_with_separation && required_y > reach_with_separation) {
                new_req_y = ((2.0 * required_area / reach_with_separation) + 2.0 * reach_with_separation) / (2.0 * aspect_ratio_ + 2.0);
                new_req_x = aspect_ratio_ * new_req_y;
            } else {
                new_req_x = required_x;
                new_req_y = required_y;
            }
            
            // Round up to the nearest multiple of bonding pitch
            new_req_x = std::ceil(new_req_x / bonding_pitch) * bonding_pitch;
            new_req_y = std::ceil(new_req_y / bonding_pitch) * bonding_pitch;
            
            if (new_req_x > current_x) {
                current_x = new_req_x;
            }
            if (new_req_y > current_y) {
                current_y = new_req_y;
            }
        }
    }
    
    // Calculate the final grid size
    FloatType required_area = area_per_pad * num_pads;
    IntType grid_x = 0;
    IntType grid_y = 0;
    
    if (required_area <= current_x * current_y) {
        grid_x = static_cast<IntType>(std::ceil(current_x / bonding_pitch));
        grid_y = static_cast<IntType>(std::ceil(current_y / bonding_pitch));
    } else {
        // Expand shorter side until sides are the same length, then expand both
        if (current_x < current_y) {
            // Y is larger
            if (current_y * current_y <= required_area) {
                grid_y = static_cast<IntType>(std::ceil(current_y / bonding_pitch));
                grid_x = static_cast<IntType>(std::ceil((required_area / current_y) / bonding_pitch));
            } else {
                FloatType required_side = std::sqrt(required_area);
                grid_x = static_cast<IntType>(std::ceil(required_side / bonding_pitch));
                grid_y = grid_x;
            }
        } else if (current_y < current_x) {
            // X is larger
            if (current_x * current_x <= required_area) {
                grid_x = static_cast<IntType>(std::ceil(current_x / bonding_pitch));
                grid_y = static_cast<IntType>(std::ceil((required_area / current_x) / bonding_pitch));
            } else {
                FloatType required_side = std::sqrt(required_area);
                grid_x = static_cast<IntType>(std::ceil(required_side / bonding_pitch));
                grid_y = grid_x;
            }
        } else {
            // Both are the same size
            FloatType required_side = std::sqrt(required_area);
            grid_x = static_cast<IntType>(std::ceil(required_side / bonding_pitch));
            grid_y = grid_x;
        }
    }
    
    FloatType pad_area = grid_x * grid_y * area_per_pad;
    return pad_area;
}

FloatType Chip::QualityYield() const {
    // Calculate quality yield based on the quality of all component chips
    FloatType quality_yield = 1.0;
    for (const auto& chip : chips_) {
        if (chip) {
            quality_yield *= chip->GetQuality();
        }
    }
    return quality_yield;
}

// Add the implementation for GetSignalCount with vector<String>
std::pair<IntType, std::map<String, IntType>> Chip::GetSignalCount(const std::vector<String>& internal_block_list) const {
    IntType signal_count = 0;
    // This is a dictionary where the key is the reach and the value is the number of signals with that reach
    std::map<String, IntType> signal_with_reach_count;

    // Find block index and internal block list indices
    IntType block_index = -1;
    std::vector<IntType> internal_block_list_indices;
    
    for (IntType i = 0; i < static_cast<IntType>(block_names_.size()); i++) {
        if (block_names_[i] == name_) {
            block_index = i;
        }
        
        // Check if block_names_[i] is in internal_block_list
        if (std::find(internal_block_list.begin(), internal_block_list.end(), block_names_[i]) != internal_block_list.end()) {
            internal_block_list_indices.push_back(i);
        }
    }
    // If chip name not found in block list, return zeros
    if (block_index == -1) {
        return {0, {}};
    }
    
    // Process each IO type in the adjacency matrix
    for (const auto& matrix_entry : global_adjacency_matrix_) {
        const String& io_type = matrix_entry.first;
        const auto& connection_matrix = matrix_entry.second;
        
        // Find the IO with matching type
        std::shared_ptr<design::IO> matching_io;
        for (const auto& io : io_list_) {
            if (io->GetType() == io_type) {
                matching_io = io;
                break;
            }
        }
        
        if (!matching_io) {
            continue; // Skip if no matching IO found
        }
        
        // Set bidirectional factor
        FloatType bidirectional_factor = matching_io->GetBidirectional() ? 0.5 : 1.0;
        
        // Process connections for this block
        for (IntType j = 0; j < static_cast<IntType>(block_names_.size()); j++) {
            // Skip if j is in internal_block_list_indices (but need to do this for 3D)
            //if (std::find(internal_block_list_indices.begin(), internal_block_list_indices.end(), j) != internal_block_list_indices.end()) {
            //    continue;
            //}

            // Get connection value from block_index to j and j to block_index
            IntType outgoing_value = 0;
            IntType incoming_value = 0;
            
            // Use 2D vector indexing to get connection values
            if (block_index < static_cast<IntType>(connection_matrix.size()) && 
                j < static_cast<IntType>(connection_matrix[block_index].size())) {
                outgoing_value = connection_matrix[block_index][j];
            }
            
            if (j < static_cast<IntType>(connection_matrix.size()) && 
                block_index < static_cast<IntType>(connection_matrix[j].size())) {
                incoming_value = connection_matrix[j][block_index];
            }
            
            // Calculate the signal count to add
            IntType count_to_add = (outgoing_value + incoming_value) * matching_io->GetWireCount();
            FloatType count_to_add_float = static_cast<FloatType>(count_to_add) * bidirectional_factor;
            IntType final_count_to_add = static_cast<IntType>(count_to_add_float);
            
            if (final_count_to_add > 0) {
                // Add to signal count
                signal_count += final_count_to_add;
                
                // Add to signal with reach count
                String reach_str = std::to_string(matching_io->GetReach());
                signal_with_reach_count[reach_str] += final_count_to_add;
            }
        }
    }
    
    return {signal_count, signal_with_reach_count};
}

IntType Chip::GetChipsSignalCount() const {
    IntType signal_count = 0;
    // Get the internal chip list
    std::vector<String> internal_chip_list = GetChipList();
    // Sum up signal counts from all child chips
    for (const auto& chip : chips_) {
        if (chip) {
            signal_count += chip->GetSignalCount(internal_chip_list).first;
        }
    }

    return signal_count;
}

std::pair<IntType, IntType> Chip::ComputeNumberReticles(FloatType area) const {
    if (!wafer_process_) {
        return {0, 0};
    }
    
    FloatType reticle_area = GetReticleX() * GetReticleY();
    IntType num_reticles = static_cast<IntType>(std::ceil(area / reticle_area));
    
    IntType largest_square_side = static_cast<IntType>(std::floor(std::sqrt(num_reticles)));
    IntType largest_square_num_reticles = largest_square_side * largest_square_side;
    
    IntType num_stitches = largest_square_side * (largest_square_side - 1) * 2 + 
                           2 * (num_reticles - largest_square_num_reticles) - 
                           static_cast<IntType>(std::ceil(static_cast<FloatType>(num_reticles - largest_square_num_reticles) / largest_square_side));
    
    return {num_reticles, num_stitches};
}

void Chip::PrintDescription() const {
    std::cout << "Chip Name: " << name_ << std::endl;
    std::cout << std::endl;
    std::cout << "Black-Box Parameters: area = " << bb_area_ << ", cost = " << bb_cost_ 
              << ", quality = " << bb_quality_ << ", power = " << bb_power_
              << ". (If any of these parameters are not empty, the value will override the computed value.)" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Chip Wafer Process: " << (wafer_process_ ? wafer_process_->GetName() : "None") << std::endl;
    std::cout << "Chip Assembly Process: " << (assembly_process_ ? assembly_process_->GetName() : "None") << std::endl;
    std::cout << "Chip Test Process: " << (test_process_ ? test_process_->GetName() : "None") << std::endl;
    
    std::cout << "Chip Stackup: ";
    for (const auto& layer : stackup_) {
        std::cout << layer.GetName() << ", ";
    }
    std::cout << std::endl;
    
    std::cout << std::endl;
    std::cout << "Chip Core Area: " << core_area_ << std::endl;
    std::cout << "Chip Stacked Area: " << GetStackedDieArea() << std::endl;
    
    std::cout << "Stacked Chips: ";
    for (const auto& chip : chips_) {
        if (chip) {
            std::cout << chip->GetName() << ", ";
        }
    }
    std::cout << std::endl;
    
    std::cout << "Chip IO Area: " << GetIoArea() << std::endl;
    std::cout << "Assembly Core Area: " << GetAssemblyCoreArea() << std::endl;
    std::cout << "Chip Buried: " << (buried_ ? "True" : "False") << std::endl;
    std::cout << "Chip Core Voltage: " << core_voltage_ << std::endl;
    
    std::cout << std::endl;
    std::cout << "Chip Area Breakdown: memory = " << fraction_memory_ 
              << ", logic = " << fraction_logic_ 
              << ", analog = " << fraction_analog_ << "." << std::endl;
    
    if (reticle_share_ != 1.0) {
        std::cout << "Chip takes up " << reticle_share_ * 100 << " % of a shared reticle" << std::endl;
    } else {
        std::cout << "Reticle is not shared." << std::endl;
    }
    
    std::cout << "NRE Cost: " << ComputeNreCost() << std::endl;
    std::cout << "Quantity: " << quantity_ << std::endl;
    
    std::cout << std::endl;
    std::cout << "Chip Power: " << power_ << std::endl;
    std::cout << "Stack Power: " << GetStackPower() << std::endl;
    std::cout << "Total Power: " << GetTotalPower() << std::endl;
    
    std::cout << std::endl;
    std::cout << "Number of Chip Power Pads: " << GetPowerPads() << std::endl;
    // Use the updated GetSignalCount function that returns a pair
    auto signal_count_result = GetSignalCount(GetChipList());
    std::cout << "Number of Signal Pads: " << signal_count_result.first << std::endl;
    std::cout << "Total number of pads: " << GetPowerPads() + signal_count_result.first << std::endl;
    
    std::cout << std::endl;
    std::cout << "Area of IO Cells: " << GetIoArea() << std::endl;
    std::cout << "Area required by pads: " << GetPadArea() << std::endl;
    std::cout << "Chip Calculated Area: " << GetArea() << std::endl;
    
    std::cout << std::endl;
    std::cout << "Chip Self True Yield: " << GetSelfTrueYield() << std::endl;
    std::cout << "Chip Self Test Yield: " << GetSelfTestYield() << std::endl;
    std::cout << "Quality Yield: " << QualityYield() << std::endl;
    
    if (assembly_process_) {
        IntType chips_len = GetChipsLen();
        FloatType stacked_die_area = GetStackedDieArea();
        
        // Use the first element of the pair returned by GetSignalCount
        std::cout << "Assembly Yield: " << assembly_process_->AssemblyYield(
            chips_len, GetChipsSignalCount(), stacked_die_area) << std::endl;
    } else {
        std::cout << "Assembly Yield: N/A (No assembly process)" << std::endl;
    }
    
    std::cout << "Chip Self Quality: " << GetSelfQuality() << std::endl;
    std::cout << "Chip Self Cost: " << GetSelfCost() << std::endl;
    std::cout << "Chip NRE Total Cost: " << ComputeNreCost() << std::endl;
    std::cout << "Chip True Yield: " << GetChipTrueYield() << std::endl;
    std::cout << "Chip Tested Yield: " << GetChipTestYield() << std::endl;
    std::cout << "Chip Quality: " << GetQuality() << std::endl;
    std::cout << "Chip Cost: " << GetCost() << std::endl;
    
    // Print description of child chips
    for (const auto& chip : chips_) {
        if (chip) {
            std::cout << ">>" << std::endl;
            chip->PrintDescription();
            std::cout << "<<" << std::endl;
        }
    }
}

FloatType Chip::GetQuality() const {
    return quality_;
}

void Chip::SetQuality(FloatType value) {
    quality_ = value;
}

FloatType Chip::GetSelfCost() const {
    return self_cost_;
}

void Chip::SetSelfCost(FloatType value) {
    self_cost_ = value;
}

FloatType Chip::GetSelfQuality() const {
    return self_quality_;
}

void Chip::SetSelfQuality(FloatType value) {
    self_quality_ = value;
}

FloatType Chip::GetChipTestYield() const {
    return chip_test_yield_;
}

void Chip::SetChipTestYield(FloatType value) {
    chip_test_yield_ = value;
}

FloatType Chip::GetChipTrueYield() const {
    return chip_true_yield_;
}

void Chip::SetChipTrueYield(FloatType value) {
    chip_true_yield_ = value;
}

FloatType Chip::GetSelfTestYield() const {
    return self_test_yield_;
}

void Chip::SetSelfTestYield(FloatType value) {
    self_test_yield_ = value;
}

FloatType Chip::GetSelfTrueYield() const {
    return self_true_yield_;
}

void Chip::SetSelfTrueYield(FloatType value) {
    self_true_yield_ = value;
}

FloatType Chip::GetCost() const {
    return cost_;
}

void Chip::SetCost(FloatType value) {
    cost_ = value;
}

FloatType Chip::GetMaskCost() const {
    FloatType cost = 0.0;
    
    // Iterate through all layers in the stackup
    for (const auto& layer : stackup_) {
        cost += layer.GetMaskCost();
    }
    
    // Multiply by reticle share
    cost *= reticle_share_;
    
    return cost;
}

// Add this factory method implementation
std::shared_ptr<Chip> Chip::Create(
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
    BoolType approx_state) {
    try {
        // Create the chip instance with a shared_ptr
        std::shared_ptr<Chip> chip = std::make_shared<Chip>(
            filename,
            etree,
            parent_chip,
            wafer_process_list,
            assembly_process_list,
            test_process_list,
            layers,
            ios,
            adjacency_matrix_definitions,
            average_bandwidth_utilization,
            block_names,
            static_value
        );

        if (approx_state == true) {
            chip->SetLayerApprox();
        }
        
        chip->Initialize();
        // Now that the chip is properly managed by a shared_ptr, we can safely use shared_from_this
        // Get the root node to pass to InitializeChildChips
        pugi::xml_node root;
        if (!filename.empty() && etree == nullptr) {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file(filename.c_str());
            if (result) {
                root = doc.document_element();
            } else {
                std::cerr << "Failed to parse XML file: " << result.description() << std::endl;
            }
        } else if (filename.empty() && etree != nullptr) {
            // Use provided XML node
            root = *static_cast<pugi::xml_node*>(etree);
        } else {
            std::cerr << "Invalid arguments: both filename and etree are null or both are provided" << std::endl;
        }
        
        // Initialize child chips now that we have a valid shared_ptr
        chip->InitializeChildChips(
            root,
            wafer_process_list,
            assembly_process_list,
            test_process_list,
            layers,
            ios,
            adjacency_matrix_definitions,
            average_bandwidth_utilization,
            block_names,
            static_value
        );

        return chip;
    } catch (const std::exception& e) {
        std::cerr << "Exception in Chip::Create: " << e.what() << std::endl;
        throw;
    }
}

// Add this InitializeChildChips method implementation
void Chip::InitializeChildChips(
    const pugi::xml_node& root,
    const std::vector<WaferProcess>* wafer_process_list,
    const std::vector<Assembly>* assembly_process_list,
    const std::vector<Test>* test_process_list,
    const std::vector<Layer>* layers,
    const std::vector<IO>* ios,
    const std::map<String, std::vector<std::vector<IntType>>>* adjacency_matrix_definitions,
    const std::map<String, std::vector<std::vector<FloatType>>>* average_bandwidth_utilization,
    const std::vector<String>* block_names,
    BoolType static_value) {
    
    // Recursively handle child chips
    for (pugi::xml_node child : root.children()) {
        if (String(child.name()).find("chip") != String::npos) {
            // Create child chip with this as parent
            // Now we can safely use shared_from_this() since the parent chip is managed by a shared_ptr
            auto* adj_matrix_ptr = reinterpret_cast<const std::map<String, std::vector<std::vector<IntType>>>*>(adjacency_matrix_definitions);
            
            // Use the factory method to create the child chip
            std::shared_ptr<Chip> child_chip = Chip::Create(
                DEFAULT_STRING_VALUE,
                &child,
                shared_from_this(), // Now safe to use
                wafer_process_list,
                assembly_process_list,
                test_process_list,
                layers,
                ios,
                adj_matrix_ptr,
                average_bandwidth_utilization,
                block_names,
                static_value
            );
            
            chips_.push_back(child_chip);
        }
    }

    // need to recompute stack power, io power, and total power
    this->SetStackPower(this->ComputeStackPower());
    this->SetIoPower(this->GetSignalPower(this->GetChipList()));
    this->SetTotalPower(this->GetPower() + this->GetIoPower() + this->GetStackPower());
    this->SetArea();
    this->SetChipTrueYield(this->ComputeChipYield());

    if (this->test_process_) {
        this->SetChipTestYield(this->test_process_->ComputeAssemblyTestYield(this->GetChipTrueYield()));
        this->SetQuality(this->test_process_->ComputeAssemblyQuality(this->GetChipTrueYield(), this->GetChipTestYield()));
    }
   
    this->SetSelfCost(this->ComputeSelfCost());
    this->SetCost(this->ComputeCost());
}

void Chip::SetLayerApprox() {
    for (auto& layer : stackup_) {
        layer.SetApprox(true);
    }
}

void Chip::ResetLayerApprox() {
    for (auto& layer : stackup_) {
        layer.SetApprox(false);
    }
}

} // namespace design 
