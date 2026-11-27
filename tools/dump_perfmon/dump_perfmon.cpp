#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct CSVModelFilename {
    std::string model_cpuid;
    std::string filename;
};

std::optional<CSVModelFilename> get_model_filename(const std::string& csv_line) {
    std::optional<CSVModelFilename> res = std::nullopt;

    std::stringstream ss{csv_line};
    std::string model_cpuid, version, filename;

    const auto& res_field0 = std::getline(ss, model_cpuid, ',');
    [[maybe_unused]] const auto& res_field1 = std::getline(ss, version, ',');
    const auto& res_field2 = std::getline(ss, filename, ',');

    if (res_field0 && res_field2) {
        res = {model_cpuid, filename};
    }

    return res;
}

std::optional<std::string> find_model_json(const std::string& perfmon_dir, const std::string& model_cpuid) {
    const auto map_file_path = perfmon_dir + "/mapfile.csv";
    std::ifstream map_file(map_file_path);
    if (!map_file.is_open()) {
        std::cerr << "Error: Could not open mapfile.csv: " << map_file_path << "\n";
        return std::nullopt;
    }

    std::string line;
    while (std::getline(map_file, line)) {
        const auto parsed_csv_line = get_model_filename(line);
        if (!parsed_csv_line) {
            throw std::runtime_error(std::string("Malformed mapfile.csv at line: ") + line);
        }

        const auto& val = parsed_csv_line.value();
        if (val.model_cpuid == model_cpuid) {
            return {val.filename};
        }
    }

    return std::nullopt;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <perfmon_dir> <model_cpuid> <map_name> <out_header>\n";
        return 1;
    }

    const std::string perfmon_dir  = argv[1];
    const std::string model_cpuid  = argv[2];
    const std::string map_name    = argv[3];
    const std::string output_path = argv[4];

    // find the model in the mapfile.csv
    const auto model_json_path = find_model_json(perfmon_dir, model_cpuid);
    if (!model_json_path) {
        std::cerr << "Could not find the model " << model_cpuid
            << " in the perfmon mapfile.csv at "
            << perfmon_dir << "\n";
        return 1;
    }

    const auto input_path = perfmon_dir + "/" + model_json_path.value();
    std::ifstream in_file(input_path);
    if (!in_file.is_open()) {
        std::cerr << "Error: Could not open input JSON: " << input_path << "\n";
        return 1;
    }

    json doc;
    try {
        in_file >> doc;
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in " << input_path << ": " << e.what() << "\n";
        return 1;
    }

    if (!doc.contains("Events") || !doc["Events"].is_array()) {
        std::cerr << "Error: JSON missing 'Events' array in " << input_path << "\n";
        return 1;
    }

    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
        std::cerr << "Error: Could not open output header: " << output_path << "\n";
        return 1;
    }

    out_file << "// Auto-generated from " << input_path << " - DO NOT EDIT\n";
    out_file << "#pragma once\n";
    out_file << "#include \"events_map.hpp\"\n";
    out_file << "#include \"namespace_name.hpp\"\n\n";

    out_file << "namespace PERF_RECORDS_NAMESPACE_NAME {\n";
    out_file << "inline const PerfEventMap_t " << map_name << " = {\n";

    for (const auto& ev : doc["Events"]) {
        // Skip entries missing required fields (some Intel perfmon JSONs have metadata/null entries)
        if (!ev.contains("EventName") || !ev.contains("EventCode") || !ev.contains("UMask")) {
            continue;
        }
        if (!ev["EventName"].is_string() || !ev["EventCode"].is_string() || !ev["UMask"].is_string()) {
            continue;
        }

        const std::string name = ev["EventName"].get<std::string>();
        const std::string code = ev["EventCode"].get<std::string>();
        const std::string mask = ev["UMask"].get<std::string>();

        if (mask.find(",") != std::string::npos) {
            std::cerr << "Skipping the event with more than one mask:"
                << " " << name << " " << mask << "\n";
            continue;
        }

        if (name.empty() || code.empty() || mask.empty()) {
            continue;
        }

        out_file << "    { \"" << name << "\","
            << " {.perf_config = RawEventConfig(" << code << ", " << mask << "),"
            << " .perf_type = PERF_TYPE_RAW}"
            << " },\n";
    }

    out_file << "};\n";
    out_file << "};  // namespace PERF_RECORDS_NAMESPACE_NAME\n";
    return 0;
}
