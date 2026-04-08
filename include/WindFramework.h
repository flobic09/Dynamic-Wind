#pragma once

#include "AnimationHandler.h"
#include "BaseObjSwapHandler.h"
#include "ModelSwapHandler.h"
#include "PushHandler.h"
#include "RotationHandler.h"
#include "TreeHandler.h"
#include "VisibilityHandler.h"

#include "REX/REX.h"

#include <unordered_set>
#include <shared_mutex>

struct WindObjectConfigs {
    const AnimationConfig* animationConfig = nullptr;
    const BaseObjSwapConfig* baseObjSwapConfig = nullptr;
    const ModelSwapConfig* modelSwapConfig = nullptr;
    const PushConfig* pushConfig = nullptr;
    const RotationConfig* rotationConfig = nullptr;
    const TreeConfig* treeConfig = nullptr;
    const VisibilityConfig* visibilityConfig = nullptr;
};

class WindFramework : public REX::Singleton<WindFramework> {
public:
    void Update(float strength, float angle) {
        treeHandler_.Update(strength, angle);

        std::unique_lock lock(mutex_);
        _trackedRefs.erase(std::remove_if(_trackedRefs.begin(), _trackedRefs.end(),
                                          [&](auto& refHandle) {
                                              auto ref = refHandle.get().get();
                                              if (!ref) {
                                                  return true;  // remove
                                              }

                                              animationHandler_.Apply(ref, strength, angle);
                                              rotationHandler_.Apply(ref, strength, angle);
                                              visibilityHandler_.Apply(ref, strength, angle);
                                              pushHandler_.Apply(ref, strength, angle);

                                              return false;  // keep
                                          }),
                           _trackedRefs.end());
    }

    void NewTargets(float strength, float angle) {
        std::unique_lock lock(mutex_);
        _trackedRefs.erase(std::remove_if(_trackedRefs.begin(), _trackedRefs.end(),
                                          [&](auto& refHandle) {
                                              auto ref = refHandle.get().get();
                                              if (!ref) {
                                                  return true;  // remove
                                              }
                                              rotationHandler_.Apply(ref, strength, angle, true);
                                              //modelSwapHandler_.Apply(ref, strength, angle);
                                              baseObjSwapHandler_.Apply(ref, strength, angle);
                                              return false;  // keep
                                          }),
                           _trackedRefs.end());
    }

    void RefLoad(RE::TESObjectREFR* ref, float targetStrength, float targetAngle) {
        if (!ref) return;

        auto baseObj = ref->GetBaseObject();
        if (!baseObj) return;

        auto baseFormID = baseObj->GetFormID();
        auto formID = ref->GetFormID();

        std::unique_lock lock(mutex_);
        if (animationHandler_.HasConfig(baseFormID) || animationHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        } else if (rotationHandler_.HasConfig(baseFormID) || rotationHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        } else if (visibilityHandler_.HasConfig(baseFormID) || visibilityHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        } else if (pushHandler_.HasConfig(baseFormID) || pushHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        } else if (modelSwapHandler_.HasConfig(baseFormID) || modelSwapHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        } else if (baseObjSwapHandler_.HasConfig(baseFormID) || baseObjSwapHandler_.HasConfig(formID)) {
            auto handle = ref->GetHandle();
            _trackedRefs.push_back(handle);
        }

        // --- other object types TBD ---
    }

    std::map<RE::FormID, WindObjectConfigs> GetConfigs() {
        std::map<RE::FormID, WindObjectConfigs> result;

        // Animation
        for (const auto& [formID, cfg] : animationHandler_.GetConfigs()) {
            result[formID].animationConfig = &cfg;
        }
        // BaseObjSwap
        for (const auto& [formID, cfg] : baseObjSwapHandler_.GetConfigs()) {
            result[formID].baseObjSwapConfig = &cfg;
        }
        // ModelSwap
        for (const auto& [formID, cfg] : modelSwapHandler_.GetConfigs()) {
            result[formID].modelSwapConfig = &cfg;
        }
        // Push
        for (const auto& [formID, cfg] : pushHandler_.GetConfigs()) {
            result[formID].pushConfig = &cfg;
        }
        // Rotation
        for (const auto& [formID, cfg] : rotationHandler_.GetConfigs()) {
            result[formID].rotationConfig = &cfg;
        }
        // Tree
        for (const auto& [formID, cfg] : treeHandler_.GetConfigs()) {
            result[formID].treeConfig = &cfg;
        }
        // Visibility
        for (const auto& [formID, cfg] : visibilityHandler_.GetConfigs()) {
            result[formID].visibilityConfig = &cfg;
        }

        return result;
    }

    void AddNewTreeConfig(const RE::TESObjectTREE* tree, const TreeDataConfig& cfg) {
        treeHandler_.AddNewConfig(tree, cfg);
    }
    TreeConfig GetTreeConfig(const RE::FormID formID) { return treeHandler_.GetConfig(formID); }
    bool HasTreeConfig(const RE::FormID formID) { return treeHandler_.HasConfig(formID); }
    void RemoveTreeConfig(const RE::FormID formID) { treeHandler_.RemoveConfig(formID); }

    void AddNewAnimationConfig(const RE::FormID formID, float minAnimSpeed, float maxAnimSpeed, float headingRotation,
                               float angleFactor) {
        animationHandler_.AddNewConfig(formID, minAnimSpeed, maxAnimSpeed, headingRotation, angleFactor);
    }
    void AddNewAnimationConfig(const RE::TESBoundObject* obj, float minAnimSpeed, float maxAnimSpeed,
                               float headingRotation, float angleFactor) {
        animationHandler_.AddNewConfig(obj->GetFormID(), minAnimSpeed, maxAnimSpeed, headingRotation, angleFactor);
    }
    void AddNewAnimationConfig(const RE::TESObjectREFR* ref, float minAnimSpeed, float maxAnimSpeed,
                               float headingRotation, float angleFactor) {
        if (!ref->IsDynamicForm()) {
            animationHandler_.AddNewConfig(ref->GetFormID(), minAnimSpeed, maxAnimSpeed, headingRotation, angleFactor);
        }
    }
    void RemoveAnimationConfig(const RE::FormID formID) { animationHandler_.RemoveConfig(formID); }

    void AddNewRotationConfig(const RE::FormID formID, float headingRotation,
                              std::optional<std::vector<float>> allowedAngles = std::nullopt) {
        rotationHandler_.AddNewConfig(formID, headingRotation, allowedAngles);
    }
    void AddNewRotationConfig(const RE::TESBoundObject* obj, float headingRotation,
                              std::optional<std::vector<float>> allowedAngles = std::nullopt) {
        rotationHandler_.AddNewConfig(obj->GetFormID(), headingRotation, allowedAngles);
    }
    void AddNewRotationConfig(const RE::TESObjectREFR* ref, float headingRotation,
                              std::optional<std::vector<float>> allowedAngles = std::nullopt) {
        if (!ref->IsDynamicForm()) {
            rotationHandler_.AddNewConfig(ref->GetFormID(), headingRotation, allowedAngles);
        }
    }
    void RemoveRotationConfig(const RE::FormID formID) { rotationHandler_.RemoveConfig(formID); }

    void AddNewVisibilityConfig(const RE::FormID formID, float minVisibility, float maxVisibility,
                                float minWindStrength, float maxWindStrength, float headingRotation,
                                float angleFactor) {
        visibilityHandler_.AddNewConfig(formID, minVisibility, maxVisibility, minWindStrength, maxWindStrength,
                                        headingRotation, angleFactor);
    }
    void AddNewVisibilityConfig(const RE::TESBoundObject* obj, float minVisibility, float maxVisibility,
                                float minWindStrength, float maxWindStrength, float headingRotation,
                                float angleFactor) {
        visibilityHandler_.AddNewConfig(obj->GetFormID(), minVisibility, maxVisibility, minWindStrength,
                                        maxWindStrength,
                                        headingRotation, angleFactor);
    }
    void AddNewVisibilityConfig(const RE::TESObjectREFR* ref, float minVisibility, float maxVisibility,
                                float minWindStrength, float maxWindStrength, float headingRotation, float angleFactor) {
        if (!ref->IsDynamicForm()) {
            visibilityHandler_.AddNewConfig(ref->GetFormID(), minVisibility, maxVisibility, minWindStrength,
                                            maxWindStrength,
                                            headingRotation, angleFactor);
        }
    }
    void RemoveVisibilityConfig(const RE::FormID formID) { visibilityHandler_.RemoveConfig(formID); }
    
    void AddNewPushConfig(const RE::FormID formID, float windSensitivity) {
        pushHandler_.AddNewConfig(formID, windSensitivity);
    }
    void AddNewPushConfig(const RE::TESBoundObject* obj, float windSensitivity) {
        pushHandler_.AddNewConfig(obj->GetFormID(), windSensitivity);
    }
    void AddNewPushConfig(const RE::TESObjectREFR* ref, float windSensitivity) {
        if (!ref->IsDynamicForm()) {
            pushHandler_.AddNewConfig(ref->GetFormID(), windSensitivity);
        }
    }
    void RemovePushConfig(const RE::FormID formID) { pushHandler_.RemoveConfig(formID); }

    void AddNewModelSwapConfig(const RE::FormID formID, float headingRotation, float angleFactor,
                               std::vector<ModelSwapEntry>& swaps) {
        modelSwapHandler_.AddNewConfig(formID, headingRotation, angleFactor, swaps);
    }
    void AddNewModelSwapConfig(const RE::TESBoundObject* obj, float headingRotation, float angleFactor,
                               std::vector<ModelSwapEntry>& swaps) {
        modelSwapHandler_.AddNewConfig(obj->GetFormID(), headingRotation, angleFactor, swaps);
    }
    void AddNewModelSwapConfig(const RE::TESObjectREFR* ref, float headingRotation, float angleFactor,
                               std::vector<ModelSwapEntry>& swaps) {
        if (!ref->IsDynamicForm()) {
            modelSwapHandler_.AddNewConfig(ref->GetFormID(), headingRotation, angleFactor, swaps);
        }
    }
    void RemoveModelSwapConfig(const RE::FormID formID) { modelSwapHandler_.RemoveConfig(formID); }

    void AddNewBaseObjSwapConfig(const std::vector<BaseObjSwapEntry>& swaps, float headingRotation, float angleFactor) {
        baseObjSwapHandler_.AddNewConfig(swaps, headingRotation, angleFactor);
    }
    void RemoveBaseObjSwapConfig(const RE::FormID formID) { baseObjSwapHandler_.RemoveConfig(formID); }

private:
    AnimationHandler animationHandler_;
    BaseObjSwapHandler baseObjSwapHandler_;
    ModelSwapHandler modelSwapHandler_;
    PushHandler pushHandler_;
    RotationHandler rotationHandler_;
    TreeHandler treeHandler_;
    VisibilityHandler visibilityHandler_;

    std::shared_mutex mutex_;
    std::vector<RE::ObjectRefHandle> _trackedRefs;
};