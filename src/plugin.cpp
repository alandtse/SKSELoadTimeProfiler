#include "Hooks.h"
#include "Logger.h"
#include "MCP.h"
#include "Localization.h"
#include "MessagingProfiler.h"
#include "Settings.h"

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    MessagingProfiler::SetRegisterSpanStartNow();
    SetupLog();
    SKSE::Init(skse);
    Localization::Load();
    Hooks::Install();
    Settings::Load();
    MessagingProfiler::Install();
    logger::info("Plugin loaded");
    MCP::Register();

    return true;
}