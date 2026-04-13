#pragma once

#include "REX/REX.h"

class Config : public REX::Singleton<Config> {
public:
    void LoadIni() {
        std::ifstream file("Data\\SKSE\\Plugins\\DynamicWind\\DynamicWind.ini");
        if (!file.is_open()) {
            logger::error("INI not found");
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.contains("bModActive")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    ModActive = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bAnimationHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    AnimationHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bBaseObjSwapHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    BaseObjSwapHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bModelSwapHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    ModelSwapHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bPushHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    PushHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bRotationHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    RotationHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bTreeHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    TreeHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
            if (line.contains("bVisibilityHandlerEnabled")) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    VisibilityHandlerEnabled = (value == "true" || value == "True" || value == "1");
                }
            }
        }
    }

    void SaveIni() {
        std::ofstream file("Data\\SKSE\\Plugins\\DynamicWind\\DynamicWind.ini");
        if (!file.is_open()) {
            logger::error("Failed to save INI");
            return;
        }
        file << "bModActive=" << (ModActive ? "true" : "false") << std::endl;
        file << "bAnimationHandlerEnabled=" << (AnimationHandlerEnabled ? "true" : "false") << std::endl;
        file << "bBaseObjSwapHandlerEnabled=" << (BaseObjSwapHandlerEnabled ? "true" : "false") << std::endl;
        file << "bModelSwapHandlerEnabled=" << (ModelSwapHandlerEnabled ? "true" : "false") << std::endl;
        file << "bPushHandlerEnabled=" << (PushHandlerEnabled ? "true" : "false") << std::endl;
        file << "bRotationHandlerEnabled=" << (RotationHandlerEnabled ? "true" : "false") << std::endl;
        file << "bTreeHandlerEnabled=" << (TreeHandlerEnabled ? "true" : "false") << std::endl;
        file << "bVisibilityHandlerEnabled=" << (VisibilityHandlerEnabled ? "true" : "false") << std::endl;
    }

    bool ModActive{true};
    bool AnimationHandlerEnabled{true};
    bool BaseObjSwapHandlerEnabled{true};
    bool ModelSwapHandlerEnabled{false};
    bool PushHandlerEnabled{true};
    bool RotationHandlerEnabled{true};
    bool TreeHandlerEnabled{true};
    bool VisibilityHandlerEnabled{true};
    bool EnableTimeLogging{false};

};