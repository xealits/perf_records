#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input.json> <output.hpp> <map_name>\n";
        return 1;
    }

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];
    const std::string map_name    = argv[3];

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
    out_file << "#include \"events_map.hpp\"\n\n";
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

        if (name.empty() || code.empty() || mask.empty()) {
            continue;
        }

        out_file << "    { \"" << name << "\", {.perf_config = RawEventConfig("
                 << code << ", " << mask << "), .perf_type = PERF_TYPE_RAW} },\n";
    }

    out_file << "};\n";
    return 0;
}
