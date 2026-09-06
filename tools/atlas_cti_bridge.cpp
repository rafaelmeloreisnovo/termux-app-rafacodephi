// Termux adapter to the producer-owned CTI API. No model/weight operations.
// Build against a separately authorized, hash-pinned llamaRafaelia checkout.
#include "cti_memory.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>

using json = nlohmann::json;

int main() {
    try {
        std::string input;
        char byte;
        while (std::cin.get(byte)) {
            if (input.size() >= 16384) return 2;
            input.push_back(byte);
        }
        const auto request = json::parse(input);
        cti_memory::config cfg;
        cfg.enabled = request.at("enabled").get<bool>();
        cfg.dir = request.at("directory").get<std::string>();
        cfg.top_k = request.at("top_k").get<int>();
        if (cfg.top_k < 1 || cfg.top_k > 20) return 2;
        // No executable from the corpus directory is allowed to be invoked.
        cfg.search_bin = "/__atlas_no_external_search_binary__";
        const std::string query = request.at("query").get<std::string>();
        if (query.empty() || query.size() > 4096) return 2;
        const auto result = cti_memory::retrieve(query, cfg);
        json hits = json::array();
        for (const auto & hit : result.hits) {
            json message_ids = json::array();
            std::ifstream rows(cfg.dir + "/omega_msgs.jsonl");
            std::string line;
            while (std::getline(rows, line)) {
                const auto row = json::parse(line);
                if (row.at("conv_i").get<int>() != hit.conv_i) continue;
                const auto safe = cti_privacy::sanitize(row.at("text").get<std::string>(), cfg.privacy);
                if (safe.result != cti_privacy::action::block && safe.output == hit.text &&
                    row.at("role").get<std::string>() == hit.role) {
                    message_ids.push_back(row.at("msg_id"));
                }
            }
            hits.push_back({{"conv_i", hit.conv_i}, {"role", hit.role},
                {"text", hit.text}, {"score", hit.score}, {"message_ids", message_ids},
                {"privacy_gate_applied", hit.privacy_gate_applied}});
        }
        const json output = {{"status", result.status},
            {"privacy_gate_applied", result.privacy_gate_applied},
            {"context_allowed", result.context_allowed}, {"source", result.source},
            // The adapter assembles UTF-8-safe bounded chunks from these hits.
            // Do not serialize the unused upstream byte-truncated context block.
            {"hits", hits},
            {"privacy_blocked_hits", result.privacy_blocked_hits},
            {"privacy_redacted_hits", result.privacy_redacted_hits}};
        std::cout << output.dump() << '\n';
        return result.ok ? 0 : 1;
    } catch (...) {
        // Exceptions can contain private input; expose only the error class.
        std::cerr << "ATLAS_CTI_BRIDGE_INVALID_INPUT\n";
        return 2;
    }
}
