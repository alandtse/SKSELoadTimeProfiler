#include "MCP.h"
#include "Logger.h"
#include "MessagingProfiler.h"
#include "Settings.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        MessagingProfiler::Dump();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    SKSE::Init(skse);
    LogSettings::Load();
    MessagingProfiler::Install();
    logger::info("Plugin loaded");
    MCP::Register();

    if (!SKSE::GetMessagingInterface()->RegisterListener(&OnMessage)) {
        SKSE::stl::report_and_fail("Failed to register message listener");
    }
    return true;
}