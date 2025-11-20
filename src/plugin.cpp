#include "MCP.h"
#include "Logger.h"
#include "MessagingProfiler.h"
#include "Settings.h"
#include "Hooks.h"

void OnMessage(SKSE::MessagingInterface::Message*) {
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    SKSE::Init(skse);

    Hooks::Install();
    LogSettings::Load();
    MessagingProfiler::Install();
    logger::info("Plugin loaded");
    MCP::Register();

    if (!SKSE::GetMessagingInterface()->RegisterListener(&OnMessage)) {
        SKSE::stl::report_and_fail("Failed to register message listener");
    }
    return true;
}