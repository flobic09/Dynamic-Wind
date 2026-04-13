#pragma once

#include "Utils.h"
#include <nlohmann/json.hpp>

struct AnimationConfig {
    RE::FormID formID{0};
    float speedMin{0.0f};
    float speedMax{1.0f};
    float headingRotation{0.0f};
    float angleFactor{0.0f};
    std::string filePath;
};

class AnimationHandler{
public:
    AnimationHandler() {
        for (auto& cfg : LoadAllConfigs("Data\\SKSE\\Plugins\\DynamicWind\\Animation")) {
            _configs[cfg.formID] = cfg;
        }
        logger::info("Loaded {} Animation Configs", _configs.size());
    }

    std::unordered_map<RE::FormID, AnimationConfig>& GetConfigs() { return _configs; }

    bool HasConfig(RE::FormID formID) { return _configs.contains(formID); }

    void AddNewConfig(const RE::FormID formID, float speedMin, float speedMax, float headingRotation,
                      float angleFactor) {
        AnimationConfig cfg = {formID, speedMin, speedMax, headingRotation, angleFactor};
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
                logger::info("Removed AnimationConfig file for {:08X}", formID);
            }
        }
    }

    void Apply(RE::TESObjectREFR* ref, float windStrength, float windAngle) {
        auto* node = ref->Get3D();
        if (!node) return;

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

        float speed = cfg.speedMin + ((cfg.speedMax - cfg.speedMin) * windStrength * finalFactor);

        Utils::ApplySpeedToNode(node, speed);
    }

    private:

    bool LoadConfig(const std::string& path, AnimationConfig& outConfig) {
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

        RE::FormID formID = Utils::ParseForm(j["FormID"].get<std::string>());
        if (formID == 0) {
            logger::error("Failed to parse FormID {} '{}'", j["FormID"].get<std::string>(), outConfig.formID);
            return false;
        }

        outConfig.formID = formID;
        if (outConfig.formID == 0) {
            logger::error("Failed to parse FormID {} '{}'", j["FormID"].get<std::string>(), outConfig.formID);
            return false;
        }
        if (j.contains("speedMin")) outConfig.speedMin = j["speedMin"].get<float>();
        if (j.contains("speedMax")) outConfig.speedMax = j["speedMax"].get<float>();
        if (j.contains("headingRotation")) outConfig.headingRotation = j["headingRotation"].get<float>();
        if (j.contains("angleFactor")) outConfig.angleFactor = j["angleFactor"].get<float>();
        outConfig.filePath = path;
        return true;
    }

    void SaveConfigToFile(const RE::FormID formID, const AnimationConfig& cfg) {
        std::filesystem::create_directories("Data\\SKSE\\Plugins\\DynamicWind\\Animation");
        std::string path = std::format("Data\\SKSE\\Plugins\\DynamicWind\\Animation\\{:08X}.json", formID);

            logger::info("Saving AnimationConfig for {:08X}", formID);
        
        nlohmann::json j;
        j["FormID"] = Utils::FormIDToString(formID);
        j["speedMin"] = cfg.speedMin;
        j["speedMax"] = cfg.speedMax;
        j["headingRotation"] = cfg.headingRotation;
        j["angleFactor"] = cfg.angleFactor;
        std::ofstream file(path);
        if (!file.is_open()) {
            logger::error("Failed to open file for writing: {}", path);
            return;
        }
        file << j.dump(4);
    }

    std::vector<AnimationConfig> LoadAllConfigs(const std::string& folder) {
        std::vector<AnimationConfig> configs;

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

            AnimationConfig cfg;
            if (LoadConfig(entry.path().string(), cfg)) {
                configs.push_back(cfg);
            }
        }
        return configs;
    }

    std::unordered_map<RE::FormID, AnimationConfig> _configs;
};