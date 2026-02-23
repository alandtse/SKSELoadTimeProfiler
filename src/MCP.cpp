#include "MCP.h"
#include "MessagingProfilerUI.h"

void HelpMarker(const char* label, const char* desc) {
    ImGuiMCP::ImGui::TextDisabled("%s", label);
    if (ImGuiMCP::ImGui::IsItemHovered()) {
        ImGuiMCP::ImGui::BeginTooltip();
        ImGuiMCP::ImGui::TextUnformatted(desc);
        ImGuiMCP::ImGui::EndTooltip();
    }
}


void MCP::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        logger::error("SKSEMenuFramework is not installed.");
        return;
    }
    SKSEMenuFramework::SetSection("Utilities");
    SKSEMenuFramework::AddSectionItem("Load Time Profiler", RenderProfiler);
}

void __stdcall MCP::RenderProfiler() {
    MessagingProfilerUI::State& state = MessagingProfilerUI::GetState();
    MessagingProfilerUI::Render(state, profilerWarnMs, profilerCritMs, showDllEntries, showEspEntries);
}