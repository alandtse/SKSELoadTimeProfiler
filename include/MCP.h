#pragma once
#include "Events.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"

static void HelpMarker(const char* desc);

namespace MCP {

	inline std::string log_path = Utilities::GetLogPath().string();
    inline std::vector<std::string> logLines;

	void Register();

    void __stdcall RenderLog();
};