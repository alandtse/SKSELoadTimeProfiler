#include "Hooks.h"
#include "Logger.h"
#include "MCP.h"
#include "MessagingProfiler.h"
#include "Settings.h"

namespace {
    std::atomic g_haveStart{false};
    std::chrono::steady_clock::time_point g_loadStart;

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void OnPostLoad(SKSE::MessagingInterface::Message* m) {
        if (m->type != SKSE::MessagingInterface::kPostLoad) return;
        if (!g_haveStart.load(std::memory_order_relaxed)) return;
        const auto t1 = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration<double, std::milli>(t1 - g_loadStart).count();
        MCP::loadTimeMs.store(ms, std::memory_order_relaxed);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    MCP::loadTimeMs.store(-1.0, std::memory_order_relaxed);
    g_loadStart = std::chrono::steady_clock::now();
    g_haveStart.store(true, std::memory_order_relaxed);
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnPostLoad);
    Hooks::Install();
    Settings::Load();
    MessagingProfiler::Install();
    logger::info("Plugin loaded");
    MCP::Register();

    return true;
}