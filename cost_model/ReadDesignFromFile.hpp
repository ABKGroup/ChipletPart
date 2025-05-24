std::tuple<
    std::map<std::string, std::vector<std::vector<int>>>,
    std::map<std::string, std::vector<std::vector<double>>>,
    std::vector<std::string>
> GlobalAdjacencyMatrixFromFile(const std::string& filename, const std::vector<design::IO>& io_list);

std::shared_ptr<design::Chip> CreateChipFromXML(
    pugi::xml_node etree,
    const std::vector<design::WaferProcess>& wafer_process_list,
    const std::vector<design::Assembly>& assembly_process_list,
    const std::vector<design::Test>& test_process_list,
    const std::vector<design::Layer>& layer_list,
    const std::vector<design::IO>& io_list,
    const std::map<std::string, std::vector<std::vector<int>>>& global_adjacency_matrix,
    const std::map<std::string, std::vector<std::vector<double>>>& average_bandwidth_utilization,
    const std::vector<std::string>& block_names); 