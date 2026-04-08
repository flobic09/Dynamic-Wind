#pragma once

#include <nlohmann/json.hpp>

#include "Utils.h"

struct BaseObjSwapEntry {
    float strength{1.0f};
    RE::TESForm* BaseObject{nullptr};
};

struct BaseObjSwapConfig {
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

    void AddNewConfig(const std::vector<BaseObjSwapEntry>& swaps, float headingRotation, float angleFactor) {
        if (swaps.empty()) return;

        auto ref = dynamic_cast<RE::TESObjectREFR*>(swaps[0].BaseObject);
        if (ref) {
            BaseObjSwapConfig cfg;
            cfg.headingRotation = headingRotation;
            cfg.angleFactor = angleFactor;
            std::vector<BaseObjSwapEntry> newSwaps;
            newSwaps.push_back({0.0f, ref});
            newSwaps.push_back({swaps[0].strength, ref->GetBaseObject()});
            // Add the rest of the swaps with their original strength
            for (size_t i = 1; i < swaps.size(); i++) {
                newSwaps.push_back(swaps[i]);
            }
            cfg.swaps = newSwaps;
            _configs[ref->GetFormID()] = cfg;
            SaveConfigToFile(ref->GetFormID(), cfg);
            return;
        }
        
        for (const auto& swap : swaps) {
            if (auto obj = swap.BaseObject) {
                BaseObjSwapConfig cfg;
                cfg.headingRotation = headingRotation;
                cfg.angleFactor = angleFactor;
                cfg.swaps = swaps;
                _configs[obj->GetFormID()] = cfg;
                SaveConfigToFile(obj->GetFormID(), cfg);
            }
        }
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
        auto baseFormID = ref->GetBaseObject()->GetFormID();
        auto it = _configs.find(baseFormID);
        if (it == _configs.end()) {
            return;
        }

        const auto& cfg = it->second;

        float objectAngle = ref->GetAngle().z + cfg.headingRotation;

        float delta = windAngle + objectAngle;

        while (delta > M_PI) delta -= M_PI*2.0f;
        while (delta < -M_PI) delta += M_PI * 2.0f;

        float alignment = std::cos(delta);

        float finalFactor = 1.0f;

        if (cfg.angleFactor > 0.0f) {
            finalFactor = 1.0f - cfg.angleFactor + (alignment * cfg.angleFactor);
        }

        for (const auto& entry : cfg.swaps) {
            if (windStrength * finalFactor <= entry.strength) {
                if (!entry.BaseObject) {
                    logger::error("Invalid BaseObject in BaseObjSwapConfig for {:08X}", baseFormID);
                    break;
                }
                if (entry.BaseObject->GetFormID() != baseFormID) {
                    Utils::ReplaceBaseObject(ref, entry.BaseObject->As<RE::TESBoundObject>());
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
            if (!file.is_open()) continue;
            file >> j;

            if (!j.contains("swaps") || !j["swaps"].is_array()) continue;

            BaseObjSwapConfig cfg;
            cfg.headingRotation = j.value("headingRotation", 0.0f);
            cfg.angleFactor = j.value("angleFactor", 0.0f);
            cfg.filePath = entry.path().string();

            std::vector<RE::FormID> formIDs;
            bool refConfig = false;
            for (const auto& swap : j["swaps"]) {
                if (!swap.contains("strength") || !swap.contains("FormID")) continue;

                RE::TESForm* form = Utils::ParseForm(swap["FormID"].get<std::string>());
                if (!form) {
                    logger::error("Failed to parse FormID {} in file {}", swap["FormID"].get<std::string>(),
                                  entry.path().string());
                    continue;
                }

                auto ref = form->As<RE::TESObjectREFR>();
                if (ref) {
                    refConfig = true;
                }
                cfg.swaps.push_back(BaseObjSwapEntry{swap["strength"].get<float>(), form});
                formIDs.push_back(form->GetFormID());
            }
            if (refConfig) {
                // If it's a reference config, only use the first FormID for mapping
                if (!formIDs.empty()) {
                    _configs[formIDs[0]] = cfg;
                }
            } else {
                for (const auto& keyFormID : formIDs) {
                    _configs[keyFormID] = cfg;
                }
            }
        }
    }

    void SaveConfigToFile(const RE::FormID formID, const BaseObjSwapConfig& cfg) {
        std::filesystem::create_directories("Data\\SKSE\\Plugins\\DynamicWind\\BaseObjSwap");
        std::string path = std::format("Data\\SKSE\\Plugins\\DynamicWind\\BaseObjSwap\\{:08X}.json", formID);

        logger::info("Saving BaseObjSwapConfig for {:08X}", formID);

        nlohmann::json j;
        j["headingRotation"] = cfg.headingRotation;
        j["angleFactor"] = cfg.angleFactor;
        j["swaps"] = nlohmann::json::array();
        for (const auto& entry : cfg.swaps) {
            std::string formIDStr = Utils::FormIDToString(entry.BaseObject->GetFormID());
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