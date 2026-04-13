#pragma once

#define M_PI 3.14159265358979323846f

namespace Utils {
    std::string FormIDToString(RE::FormID formID);
    RE::FormID ParseForm(const std::string& str);


    void ApplySpeedToNode(RE::NiAVObject* node, float speed);

    void ApplyImpulseToNode(RE::NiAVObject* node, RE::hkVector4 velocity);

    void ReplaceModel(RE::TESObjectREFR* ref, std::string modelPath);

    void ReplaceBaseObject(RE::TESObjectREFR* ref, RE::TESBoundObject* newBase);

    std::string GetModelPath(RE::TESBoundObject* base);

    bool IsInWindCell(RE::TESObjectREFR* ref);
}