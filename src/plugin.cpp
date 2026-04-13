#include "logger.h"
#include "Hooks.h"
#include "MCP.h"
#include "Settings.h"
#include "WindFramework.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        MCP::Register();
        WindFramework::GetSingleton();  // Init
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    Hooks::InstallHooks();
    Config::GetSingleton()->LoadIni();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
