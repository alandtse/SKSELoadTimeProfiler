#pragma once

namespace Utilities {
    const auto mod_name = "Mod Control Panel Utilities";
    const auto plugin_version = SKSE::PluginDeclaration::GetSingleton()->GetVersion();

    std::filesystem::path GetLogPath();

    std::vector<std::string> ReadLogFile();

    std::optional<std::uint32_t> hex_to_u32(std::string_view s);
};