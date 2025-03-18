#include <iostream>
#include <fstream>
#include <string>
#include "Python.h"
#include "evaluator.h"

// Define a function to read the netlist xml file and extract the netlist information into netlist_item struct objects.
// For easy access, the struct objects are then stored as a vector of vectors of netlist_item struct objects.
// Two of these vectors should be created, each containing the same items, but the first will store the items by the from_block and the second by the to_block.

// Each netlist_item struct object represents a netlist item and contains the parameters, from_block, to_block, and bandwidth.
// from_block and to_block are integers based on the location of the block in the block_definitions file. bandwidth is a float.
struct netlist_item {
    int from_block;
    int to_block;
    float bandwidth;
};

int findIndex(const std::vector<std::string>& block_names, const std::string& block) {
    for (size_t i = 0; i < block_names.size(); ++i) {
        if (block_names[i] == block) {
            return static_cast<int>(i); // Found the match, return the index
        }
    }
    return -1; // Not found
}

// First declare a function to generate all the netlist_item struct objects and store them in a vector.
// This function should take in the netlist xml file and return a vector of netlist_item struct objects.
std::vector<netlist_item> readNetlist(std::string block_definitions, std::string netlist_file) {
    // Initialize the vector of netlist_item struct objects
    std::vector<netlist_item> netlist_items;
    // Open the netlist xml file
    std::ifstream file(block_definitions);
    std::string line;

    std::vector<std::string> block_names;
    // Read the file line by line
    while (std::getline(file, line)) {
        // Add the first word of each line to the block_names vector
        block_names.push_back(line.substr(0, line.find(" ")));
    }
    // Close block_definitions file
    file.close();

    std::ifstream n_file(netlist_file);
    line = "";
    // Declare a vector of strings
    std::vector<std::string> netlist_file_lines;
    // Read the file line by line and store as an array of strings.
    while (std::getline(n_file, line)) {
        netlist_file_lines.push_back(line);
    }
    // Close the netlist file
    n_file.close();

    // Iterate through the netlist_file_lines vector and each time a line contains "<net type=" add the content parsed from between quotes on the lines 1, 2, and 4 lines after to the from_block, to_block, and bandwidth parameters of a new struct respectively.
    for (int i = 0; i < netlist_file_lines.size(); i++) {
        if (netlist_file_lines[i].find("<net type=") != std::string::npos) {
            // Create a new netlist_item struct object
            netlist_item item;
            // Parse the from_block
            int from_block_name_start = netlist_file_lines[i + 1].find("\"") + 1;
            int from_block_name_end = netlist_file_lines[i + 1].rfind("\"");
            std::string from_block = netlist_file_lines[i + 1].substr(from_block_name_start, from_block_name_end - from_block_name_start);
            // Search block_names for the index where the string that matches from_block is located and set it to from_block_number
            int from_block_number = findIndex(block_names, from_block);
            // int from_block_number = std::find(block_names.begin(), block_names.end(), from_block) - block_names.begin();
            item.from_block = from_block_number;
            // Parse the to_block
            int to_block_name_start = netlist_file_lines[i + 2].find("\"") + 1;
            int to_block_name_end = netlist_file_lines[i + 2].rfind("\"");
            std::string to_block = netlist_file_lines[i + 2].substr(to_block_name_start, to_block_name_end - to_block_name_start);
            // Search block_names for the index where to_block is located and set it to to_block_number
            int to_block_number = findIndex(block_names, to_block);
            // int to_block_number = std::find(block_names.begin(), block_names.end(), to_block) - block_names.begin();
            item.to_block = to_block_number;
            // Parse the bandwidth
            item.bandwidth = std::stof(netlist_file_lines[i + 4].substr(netlist_file_lines[i + 4].find("\"") + 1, netlist_file_lines[i + 4].rfind("\"") - netlist_file_lines[i + 4].find("\"") - 1));
            // Add the netlist_item struct object to the vector
            netlist_items.push_back(item);
        }
    }

    // Return the vector of netlist_item struct objects
    return netlist_items;
}

// Then, create two vectors of vectors of netlist_item struct objects, one for from_block and one for to_block.
// This function should take in the vector of netlist_item struct objects and return a vector of vectors of netlist_item struct objects ordered by the from object.
std::vector<std::vector<netlist_item>> orderNetlistByFrom(std::vector<netlist_item> netlist_items) {
    // Initialize the vector of vectors of netlist_item struct objects. This should have a length equal to the largest from_block in the netlist_items vector.
    std::vector<std::vector<netlist_item>> netlist_items_by_from;
    // Iterate through the netlist_items vector and save the largest from_block or to_block value.
    int largest_block = 0;
    for (int i = 0; i < netlist_items.size(); i++) {
        if (netlist_items[i].from_block > largest_block) {
            largest_block = netlist_items[i].from_block;
        }
        if (netlist_items[i].to_block > largest_block) {
            largest_block = netlist_items[i].to_block;
        }
    }
    // Resize the netlist_items_by_from vector to have a length equal to the largest from_block value.
    netlist_items_by_from.resize(largest_block + 1);
    // Iterate through the netlist_items vector and add each netlist_item struct object to the netlist_items_by_from vector at the index of the from_block.
    for (int i = 0; i < netlist_items.size(); i++) {
        netlist_items_by_from[netlist_items[i].from_block].push_back(netlist_items[i]);
    }
    return netlist_items_by_from;
}

// This function should take in the vector of netlist_item struct objects and return a vector of vectors of netlist_item struct objects ordered by the to object.
std::vector<std::vector<netlist_item>> orderNetlistByTo(std::vector<netlist_item> netlist_items) {
    // Initialize the vector of vectors of netlist_item struct objects. This should have a length equal to the largest to_block in the netlist_items vector.
    std::vector<std::vector<netlist_item>> netlist_items_by_to;
    // Iterate through the netlist_items vector and save the largest from_block or to_block value.
    int largest_block = 0;
    for (int i = 0; i < netlist_items.size(); i++) {
        if (netlist_items[i].from_block > largest_block) {
            largest_block = netlist_items[i].from_block;
        }
        if (netlist_items[i].to_block > largest_block) {
            largest_block = netlist_items[i].to_block;
        }
    }
    // Resize the netlist_items_by_to vector to have a length equal to the largest to_block value.
    netlist_items_by_to.resize(largest_block + 1);
    // Iterate through the netlist_items vector and add each netlist_item struct object to the netlist_items_by_to vector at the index of the to_block.
    for (int i = 0; i < netlist_items.size(); i++) {
        netlist_items_by_to[netlist_items[i].to_block].push_back(netlist_items[i]);
    }
    return netlist_items_by_to;
}

// Get cutset bandwidth for a partition
float getCutsetBandwidth(std::vector<int> partitionIDs, int partition_to_get_cutset, std::vector<std::vector<netlist_item>> netlist_items_by_from, std::vector<std::vector<netlist_item>> netlist_items_by_to) {
    // Initialize the cutset bandwidth
    float cutset_bandwidth = 0;
    // Iterate through the partitionIDs vector.
    for (int i = 0; i < partitionIDs.size(); i++) {
        // If the partitionID is equal to partition_to_get_cutset, iterate through the netlist_items_by_from vector at the index of the block and add the bandwidth to the cutset bandwidth for each netlist_item that's to_block does not have the same partition ID as partition_to_get_cutset.
        // Do the same for getting the cutset bandwidth for the to_block.
        if (partitionIDs[i] == partition_to_get_cutset) {
            for (int j = 0; j < netlist_items_by_from[i].size(); j++) {
                if (partitionIDs[netlist_items_by_from[i][j].to_block] != partition_to_get_cutset) {
                    cutset_bandwidth += netlist_items_by_from[i][j].bandwidth;
                }
            }
            for (int j = 0; j < netlist_items_by_to[i].size(); j++) {
                if (partitionIDs[netlist_items_by_to[i][j].from_block] != partition_to_get_cutset) {
                    cutset_bandwidth += netlist_items_by_to[i][j].bandwidth;
                }
            }
        }
    }
    return cutset_bandwidth;
}


int main() {
    // Test Flags
    bool report_baseline_cost = true;
    bool test_taylor_approximation = false;
    bool report_taylor_sweep = false;
    bool test_incremental_cost = false;
    bool test_single_move_cost = false;
    bool test_file_partition = false;
    bool include_bw_taylor = false;

    float cost_coefficient = 1.0;
    float power_coefficient = 0.0;

    //////////////////////////////////////////
    // Initialize the Library Python Object //
    //////////////////////////////////////////

    // Library Files
    std::string io_file = "io_definitions.xml";
    std::string layer_file = "layer_definitions.xml";
    std::string assembly_process_file = "assembly_process_definitions.xml";
    std::string wafer_process_file = "wafer_process_definitions.xml";
    std::string test_file = "test_definitions.xml";
    // System Definition Files 
    //std::string netlist_file = "block_level_netlist.xml";
    //std::string blocks_file = "block_definitions.txt";
    std::string netlist_file = "block_level_netlist_ws-48_4_14_4_200_200.xml";
    std::string blocks_file = "block_definitions_ws-48_4_14_4_200_200.txt";

    std::cout << "Initializing" << std::endl;
    PyObject* libraryDicts = init(io_file, layer_file, wafer_process_file, assembly_process_file, test_file, netlist_file, blocks_file);

    std::cout << "Initialization complete" << std::endl;
    
    // Read the Block File for the Test Case
    std::vector<block> blocks = readBlocks(blocks_file);
    std::cout << "blocks size is " << blocks.size() << std::endl;

    // Define a vector of block areas and find the max, min, mean, and median.
    std::vector<float> block_areas;
    for (int i = 0; i < blocks.size(); i++) {
        block_areas.push_back(blocks[i].area);
        //if (blocks[i].is_memory == true) {
        //    std::cout << blocks[i].name << " is a memory block" << std::endl;
        //}
    }
    float max_area = 0.0;
    float min_area = 1000000000.0;
    float total_area = 0.0;
    for (int i = 0; i < block_areas.size(); i++) {
        if (block_areas[i] > max_area) {
            max_area = block_areas[i];
        }
        if (block_areas[i] < min_area) {
            min_area = block_areas[i];
        }
        total_area += block_areas[i];
    }
    float mean_area = total_area / block_areas.size();
    // Bubble sort the block_areas vector as the lazy option.
    for (int i = 0; i < block_areas.size(); i++) {
        for (int j = 0; j < block_areas.size() - 1; j++) {
            if (block_areas[j] > block_areas[j + 1]) {
                float temp = block_areas[j];
                block_areas[j] = block_areas[j + 1];
                block_areas[j + 1] = temp;
            }
        }
    }
    
    float median_area = block_areas[block_areas.size() / 2];
    std::cout << "Max area: " << max_area << std::endl;
    std::cout << "Min area: " << min_area << std::endl;
    std::cout << "Mean area: " << mean_area << std::endl;
    std::cout << "Median area: " << median_area << std::endl;
    float area_recalculation_threshold = mean_area;

    // Read the Netlist File for the Test Case
    std::vector<netlist_item> netlist_items = readNetlist(blocks_file, netlist_file);
    std::vector<std::vector<netlist_item>> netlist_items_by_from = orderNetlistByFrom(netlist_items);
    std::vector<std::vector<netlist_item>> netlist_items_by_to = orderNetlistByTo(netlist_items);

    // Set Partitions for Initial Test Case
    std::vector<int> partitionIDs; // Replace with your partition IDs
    int numPartitions = 5;
    // Initialize to 0,1,...,n,0,1,...,n,...
    for (int i = 0; i < blocks.size(); i++) {
        //partitionIDs.push_back(i/(blocks.size()/numPartitions));
        int tile_num = i / 48;
        int block_num = i % 48;
        if (block_num >= 2 && block_num < 6) { 
            partitionIDs.push_back(numPartitions-1);
        }
        else {
            partitionIDs.push_back(tile_num);
        }
    }
    
    // Print out partitionIDs
    std::cout << "Partition IDs: ";
    for (int i = 0; i < partitionIDs.size(); i++) {
        std::cout << partitionIDs[i] << " ";
    }
    std::cout << std::endl;

    // Define an array of tech node strings for each partition
    std::vector<std::string> tech_array;
    // Push back the tech node "45nm" for each partition
    for (int i = 0; i < numPartitions; i++) {
        tech_array.push_back("7nm");
    }
    //tech_array.push_back("45nm");
    //tech_array.push_back("10nm");

    // Define the aspect ratios for the partitions
    std::vector<float> aspect_ratios;
    // Push back the aspect ratio 1.0 for each partition
    for (int i = 0; i < numPartitions; i++) {
        aspect_ratios.push_back(1.0);
    }

    // Define the coordinates for the partitions
    std::vector<float> x_locations;
    for (int i = 0; i < numPartitions; i++) {
        x_locations.push_back(0.0);
    }
    std::vector<float> y_locations;
    for (int i = 0; i < numPartitions; i++) {
        y_locations.push_back(0.0);
    }

    if (report_baseline_cost) {
        // Test getCostFromScratch
        float costFromScratch = getCostFromScratch(partitionIDs, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, blocks, cost_coefficient, power_coefficient);
        std::cout << std::endl;
        std::cout << "Cost from scratch: " << costFromScratch << std::endl;
        float powerFromScratch = getCostFromScratch(partitionIDs, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, blocks, 0.0, 1.0);
        std::cout << "Power from scratch: " << powerFromScratch << std::endl;
        std::cout << std::endl;
    }


   // Destroy the model and database objects
    std::cout << "Destroying database" << std::endl;
    destroyDatabase(libraryDicts);
    std::cout << "Database destroyed" << std::endl;

    return 0;
}
