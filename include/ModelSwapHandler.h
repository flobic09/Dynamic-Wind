#pragma once

#include <nlohmann/json.hpp>

#include "Utils.h"

struct ModelSwapEntry {
    float strength{1.0f};
    std::string modelPath;
};

struct ModelSwapConfig {
    RE::FormID formID{0};
    float headingRotation{0.0f};
    float angleFactor{0.0f};
    std::vector<ModelSwapEntry> swaps;
    std::string filePath;
};

class ModelSwapHandler {
public:
    ModelSwapHandler() {
        for (auto& cfg : LoadAllConfigs("Data\\SKSE\\Plugins\\DynamicWind\\ModelSwap")) {
            _configs[cfg.formID] = cfg;
        }
        logger::info("Loaded {} ModelSwap Configs", _configs.size());
    }

    std::unordered_map<RE::FormID, ModelSwapConfig>& GetConfigs() { return _configs; }

    bool HasConfig(RE::FormID formID) { return _configs.contains(formID); }

    void AddNewConfig(RE::FormID formID, float headingRotation, float angleFactor, std::vector<ModelSwapEntry>& swaps) {
        ModelSwapConfig cfg = {formID, headingRotation, angleFactor, swaps};
        _configs[cfg.formID] = cfg;
        SaveConfigToFile(cfg.formID, cfg);
    }

    void RemoveConfig(RE::FormID formID) {
        auto it = _configs.find(formID);
        if (it != _configs.end()) {
            std::string path = _configs[formID].filePath;
            _configs.erase(formID);
            if (std::filesystem::exists(path)) {
                std::filesystem::remove(path);
                logger::info("Removed ModelSwapConfig file for {:08X}", formID);
            }
        }
    }

    void Apply(RE::TESObjectREFR* ref, float windStrength, float windAngle) {
        auto it = _configs.find(ref->GetFormID());
        if (it == _configs.end()) {
            it = _configs.find(ref->GetBaseObject()->GetFormID());
            if (it == _configs.end()) return;
        }

        const auto& cfg = it->second;

        float objectAngle = ref->GetAngle().z + cfg.headingRotation;

        float delta = windAngle + objectAngle;

        while (delta > M_PI) delta -= M_PI * 2.0f;
        while (delta < -M_PI) delta += M_PI * 2.0f;

        float alignment = std::cos(delta);

        float finalFactor = 1.0f;

        if (cfg.angleFactor > 0.0f) {
            finalFactor = 1.0f - cfg.angleFactor + (alignment * cfg.angleFactor);
        }

        for (const auto& entry : cfg.swaps) {
            if (windStrength * finalFactor <= entry.strength) {
                Utils::ReplaceModel(ref, entry.modelPath);
                break;
            }
        }
    }

private:
    bool LoadConfig(const std::string& path, ModelSwapConfig& outConfig) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;
        file >> j;

        if (!j.contains("FormID")) {
            logger::error("file dont contain FormID");
            return false;
        }

        auto* form = Utils::ParseForm(j["FormID"].get<std::string>());
        if (!form) {
            logger::error("Failed to parse FormID {} '{}'", j["FormID"].get<std::string>(), outConfig.formID);
            return false;
        }

        outConfig.formID = form->GetFormID();
        outConfig.headingRotation = j.value("headingRotation", 0.0f);
        outConfig.angleFactor = j.value("angleFactor", 0.0f);
        outConfig.filePath = path;

        outConfig.swaps.clear();
        if (j.contains("swaps") && j["swaps"].is_array()) {
            for (const auto& entry : j["swaps"]) {
                if (entry.contains("strength") && entry.contains("model")) {
                    outConfig.swaps.push_back(ModelSwapEntry{
                        entry["strength"].get<float>(),
                        entry["model"].get<std::string>()
                    });
                }
            }
        }

        return true;
    }

    void SaveConfigToFile(const RE::FormID formID, const ModelSwapConfig& cfg) {
        std::filesystem::create_directories("Data\\SKSE\\Plugins\\DynamicWind\\ModelSwap");
        std::string path = std::format("Data\\SKSE\\Plugins\\DynamicWind\\ModelSwap\\{:08X}.json", formID);
        logger::info("Saving ModelSwapConfig for {:08X}", formID);
        
        nlohmann::json j;
        j["FormID"] = Utils::FormIDToString(formID);
        j["headingRotation"] = cfg.headingRotation;
        j["angleFactor"] = cfg.angleFactor;
        j["swaps"] = nlohmann::json::array();
        for (const auto& entry : cfg.swaps) {
            j["swaps"].push_back({
                {"strength", entry.strength},
                {"model", entry.modelPath}
            });
        }
        std::ofstream file(path);
        if (!file.is_open()) {
            logger::error("Failed to open file for writing: {}", path);
            return;
        }
        file << j.dump(4);
    }

    std::vector<ModelSwapConfig> LoadAllConfigs(const std::string& folder) {
        std::vector<ModelSwapConfig> configs;

        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
            logger::error("{} not exists or is not directory", folder);
            return configs;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (entry.path().extension() != ".json") {
                continue;
            }

            logger::info("Reading {}", entry.path().string());

            ModelSwapConfig cfg;
            if (LoadConfig(entry.path().string(), cfg)) {
                configs.push_back(cfg);
            }
        }
        return configs;
    }

    std::unordered_map<RE::FormID, ModelSwapConfig> _configs;
};