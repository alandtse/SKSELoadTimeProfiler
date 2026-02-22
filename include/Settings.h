#pragma once
#include "MCP.h"

namespace Settings {

    std::filesystem::path GetConfigPath();

    void Load();

    void Save();
};