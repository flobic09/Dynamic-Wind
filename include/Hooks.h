#pragma once

#include <unordered_set>

namespace Hooks {

    struct UpdateHook {
        static void Update(RE::Actor* a_this, float a_delta);
        static inline REL::Relocation<decltype(Update)> Update_;
    };

    struct RefLoadHook {
        static RE::NiAVObject* Load3D(RE::TESObjectREFR* a_this, bool a_backgroundLoading);
        static inline REL::Relocation<decltype(Load3D)> Load3D_;
    };

    inline void InstallHooks() {
        auto runtime = REL::Module::GetRuntime();
        logger::info("Detected runtime: {}", runtime == REL::Module::Runtime::AE   ? "Anniversary Edition"
                                             : runtime == REL::Module::Runtime::SE ? "Special Edition"
                                             : runtime == REL::Module::Runtime::VR ? "VR"
                                                                                   : "Unknown");
        if (runtime == REL::Module::Runtime::VR) {
            UpdateHook::Update_ =
                REL::Relocation<std::uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0xAF, UpdateHook::Update);
        } else {
            UpdateHook::Update_ =
                REL::Relocation<std::uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0xAD, UpdateHook::Update);
        }
        // seems to be the same for both SE,AE and VR
        RefLoadHook::Load3D_ =
            REL::Relocation<std::uintptr_t>(RE::VTABLE_TESObjectREFR[0]).write_vfunc(0x6A, RefLoadHook::Load3D);
    }
}