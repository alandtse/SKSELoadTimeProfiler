#pragma once
#include "MCP.h"
#include "MessagingProfilerUI.h"
#include "Utils.h"

namespace LogSettings {
	inline bool log_trace = true;
	inline bool log_info = true;
	inline bool log_warning = true;
	inline bool log_error = true;

	inline std::filesystem::path GetConfigPath()
	{
		const auto plugin = SKSE::PluginDeclaration::GetSingleton()->GetName();
		// Base dir is this DLL folder: .../Data/SKSE/Plugins
		std::filesystem::path dllPath{ REL::Module::get().filename() };
		auto dir = dllPath.parent_path() / std::filesystem::path(plugin);
		std::error_code ec; std::filesystem::create_directories(dir, ec);
		return dir / std::format("{}.json", plugin);
	}

	inline void Load()
	{
		auto path = GetConfigPath();
		std::ifstream ifs(path);
		if (!ifs.is_open()) return; // defaults remain
		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document doc; doc.ParseStream(isw);
		if (doc.HasParseError() || !doc.IsObject()) return;
		if (doc.HasMember("log_trace")   && doc["log_trace"].IsBool())   log_trace   = doc["log_trace"].GetBool();
		if (doc.HasMember("log_info")    && doc["log_info"].IsBool())    log_info    = doc["log_info"].GetBool();
		if (doc.HasMember("log_warning") && doc["log_warning"].IsBool()) log_warning = doc["log_warning"].GetBool();
		if (doc.HasMember("log_error")   && doc["log_error"].IsBool())   log_error   = doc["log_error"].GetBool();
		if (doc.HasMember("profiler_warn_ms") && doc["profiler_warn_ms"].IsNumber()) MCP::profilerWarnMs = doc["profiler_warn_ms"].GetDouble();
		if (doc.HasMember("profiler_crit_ms") && doc["profiler_crit_ms"].IsNumber()) MCP::profilerCritMs = doc["profiler_crit_ms"].GetDouble();
		if (doc.HasMember("profiler_visible") && doc["profiler_visible"].IsArray()) {
			auto arr = doc["profiler_visible"].GetArray();
			auto names = MessagingProfilerBackend::GetMessageTypeNames();
			// Resize selection state to match
			MessagingProfilerUI::State tmp; MessagingProfilerUI::EnsureSelectionSize(tmp, names.size());
			std::vector<bool> vis(names.size(), false);
			for (auto& v : arr) if (v.IsString()) {
				std::string s = v.GetString();
				for (std::size_t i=0;i<names.size();++i) if (names[i]==s) vis[i]=true;
			}
			// Store into a global state instance (created lazily in UI) via a helper setter.
			MessagingProfilerUI::SetInitialVisibility(vis);
		}
	}

	inline void Save()
	{
		rapidjson::Document doc(rapidjson::kObjectType);
		auto& a = doc.GetAllocator();
		doc.AddMember("log_trace",   log_trace, a);
		doc.AddMember("log_info",    log_info, a);
		doc.AddMember("log_warning", log_warning, a);
		doc.AddMember("log_error",   log_error, a);
		doc.AddMember("profiler_warn_ms", MCP::profilerWarnMs, a);
		doc.AddMember("profiler_crit_ms", MCP::profilerCritMs, a);
		// Persist visibility
		auto names = MessagingProfilerBackend::GetMessageTypeNames();
		auto vis = MessagingProfilerUI::GetCurrentVisibility();
		rapidjson::Value arr(rapidjson::kArrayType);
		for (std::size_t i=0;i<names.size() && i<vis.size(); ++i) if (vis[i]) arr.PushBack(rapidjson::Value(names[i].data(), a), a);
		doc.AddMember("profiler_visible", arr, a);
		rapidjson::StringBuffer sb; rapidjson::Writer<rapidjson::StringBuffer> wr(sb);
		doc.Accept(wr);
		auto path = GetConfigPath();
		std::ofstream ofs(path, std::ios::trunc);
		if (!ofs.is_open()) return;
		ofs << sb.GetString();
	}
};