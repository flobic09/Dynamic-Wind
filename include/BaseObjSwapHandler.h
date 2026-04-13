#pragma once

#include <nlohmann/json.hpp>

#include "Utils.h"

struct BaseObjSwapEntry {
    float strength{1.0f};
    RE::FormID formID{0};
};

struct BaseObjSwapConfig {
    RE::FormID formID{0};
    float headingRotation{0.0f};
    float angleFactor{0.0f};
    std::vector<BaseObjSwapEntry> swaps;
    std::string filePath;
};

class BaseObjSwapHandler {
public:
    BaseObjSwapHandler() {
        LoadAllConfigs("Data\\SKSE\\Plugins\\DynamicWind\\BaseObjSwap");
        logger::info("Loaded {} BaseObjSwap Configs", _configs.size());
    }

    std::unordered_map<RE::FormID, BaseObjSwapConfig>& GetConfigs() { return _configs; }

    bool HasConfig(RE::FormID formID) { return _configs.contains(formID); }

    void AddNewConfig(RE::FormID targetFormID, const std::vector<BaseObjSwapEntry>& swaps, float headingRotation,
                      float angleFactor) {
        if (swaps.empty()) return;

        BaseObjSwapConfig cfg;
        cfg.formID = targetFormID;
        cfg.headingRotation = headingRotation;
        cfg.angleFactor = angleFactor;
        cfg.swaps = swaps;

        _configs[targetFormID] = cfg;
        SaveConfigToFile(targetFormID, cfg);
    }

    void RemoveConfig(RE::FormID formID) {
        auto it = _configs.find(formID);
        if (it != _configs.end()) {
            std::string path = _configs[formID].filePath;
            _configs.erase(formID);
            if (std::filesystem::exists(path)) {
                std::filesystem::remove(path);
                logger::info("Removed BaseObjSwapConfig file for {:08X}", formID);
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
                if (entry.formID == 0) {
                    logger::error("Invalid formID in BaseObjSwapConfig for {:08X}", entry.formID);
                    break;
                }
                if (ref->GetBaseObject()->GetFormID() != entry.formID) {
                    if (auto* newBase = RE::TESForm::LookupByID<RE::TESBoundObject>(entry.formID)) {
                        Utils::ReplaceBaseObject(ref, newBase);
                    } else {
                        logger::error("Failed to lookup form {:08X} for BaseObjSwapConfig", entry.formID);
                    }
                }
                break;
            }
        }
    }

private:
    void LoadAllConfigs(const std::string& folder) {
        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
            logger::error("{} not exists or is not directory", folder);
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

            logger::info("Reading {}", entry.path().string());

            nlohmann::json j;
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                logger::error("Failed to open file: {}", entry.path().string());
                continue;
            }
            file >> j;

            if (!j.contains("swaps") || !j["swaps"].is_array()) {
                logger::error("Invalid config format in file {}: missing 'swaps' array", entry.path().string());
                continue;
            }

            RE::FormID formID = Utils::ParseForm(j["FormID"].get<std::string>());
            if (formID == 0) {
                logger::error("Failed to parse FormID {} '{}'", j["FormID"].get<std::string>(), formID);
                continue;
            }

            BaseObjSwapConfig cfg;
            cfg.formID = formID;
            cfg.headingRotation = j.value("headingRotation", 0.0f);
            cfg.angleFactor = j.value("angleFactor", 0.0f);
            cfg.filePath = entry.path().string();

            bool refConfig = false;
            for (const auto& swap : j["swaps"]) {
                if (!swap.contains("strength") || !swap.contains("FormID")) {
                    logger::error("Invalid swap entry in file {}: missing 'strength' or 'FormID'",
                                  entry.path().string());
                    continue;
                }

                RE::FormID swapFormID = Utils::ParseForm(swap["FormID"].get<std::string>());
                if (swapFormID == 0) {
                    logger::error("Failed to parse FormID {} in file {}", swap["FormID"].get<std::string>(),
                                  entry.path().string());
                    continue;
                }

                auto form = RE::TESForm::LookupByID(swapFormID);
                if (!form || form->Is(RE::FormType::Reference)) {
                    refConfig = true;
                }
                cfg.swaps.push_back(BaseObjSwapEntry{swap["strength"].get<float>(), swapFormID});
            }
            _configs[formID] = cfg;
        }
    }

    void SaveConfigToFile(const RE::FormID formID, const BaseObjSwapConfig& cfg) {
        std::filesystem::create_directories("Data\\SKSE\\Plugins\\DynamicWind\\BaseObjSwap");
        std::string path = std::format("Data\\SKSE\\Plugins\\DynamicWind\\BaseObjSwap\\{:08X}.json", formID);

        logger::info("Saving BaseObjSwapConfig for {:08X}", formID);

        nlohmann::json j;
        j["FormID"] = Utils::FormIDToString(formID);
        j["headingRotation"] = cfg.headingRotation;
        j["angleFactor"] = cfg.angleFactor;
        j["swaps"] = nlohmann::json::array();
        for (const auto& entry : cfg.swaps) {
            std::string formIDStr = Utils::FormIDToString(entry.formID);
            j["swaps"].push_back({
                {"FormID", formIDStr},
                {"strength", entry.strength}
            });
        }
        std::ofstream file(path);
        if (!file.is_open()) {
            logger::error("Failed to open file for writing: {}", path);
            return;
        }
        file << j.dump(4);
    }

    std::unordered_map<RE::FormID, BaseObjSwapConfig> _configs;
};