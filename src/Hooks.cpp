#include "Hooks.h"
#include "WindManager.h"
#include "WindFramework.h"

namespace Hooks {
    void UpdateHook::Update(RE::Actor* a_this, float a_delta) {
        Update_(a_this, a_delta);
        auto start = std::chrono::high_resolution_clock::now();
        Wind::Manager::GetSingleton()->Update(a_this, a_delta);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        //logger::debug("Wind::Manager Update {} ms", elapsed.count());
    }

    RE::NiAVObject* RefLoadHook::Load3D(RE::TESObjectREFR* a_this, bool a_backgroundLoading) {
        auto res = Load3D_(a_this, a_backgroundLoading);
        auto start = std::chrono::high_resolution_clock::now();
        auto [windAngle, windSpeed] = Wind::Manager::GetSingleton()->GetTargets();

        WindFramework::GetSingleton()->RefLoad(a_this, windSpeed, windAngle);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        //logger::debug("WindFramework RefLoad {} ms", elapsed.count());
        return res;
    }
}