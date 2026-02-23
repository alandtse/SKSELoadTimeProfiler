#include "Hooks.h"
#include "Logger.h"
#include "MCP.h"
#include "MessagingProfiler.h"
#include "Settings.h"

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    MCP::loadTimeMs.store(-1.0, std::memory_order_relaxed);
    MessagingProfiler::SetRegisterSpanStartNow();
    SetupLog();
    SKSE::Init(skse);
    Hooks::Install();
    Settings::Load();
    MessagingProfiler::Install();
    logger::info("Plugin loaded");
    MCP::Register();

    return true;
}