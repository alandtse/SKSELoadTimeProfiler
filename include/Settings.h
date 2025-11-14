#pragma once
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
	}

	inline void Save()
	{
		rapidjson::Document doc(rapidjson::kObjectType);
		auto& a = doc.GetAllocator();
		doc.AddMember("log_trace",   log_trace, a);
		doc.AddMember("log_info",    log_info, a);
		doc.AddMember("log_warning", log_warning, a);
		doc.AddMember("log_error",   log_error, a);
		rapidjson::StringBuffer sb; rapidjson::Writer<rapidjson::StringBuffer> wr(sb);
		doc.Accept(wr);
		auto path = GetConfigPath();
		std::ofstream ofs(path, std::ios::trunc);
		if (!ofs.is_open()) return;
		ofs << sb.GetString();
	}
};