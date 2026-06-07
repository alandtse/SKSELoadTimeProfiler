#include "DevBenchBridge.h"

#ifdef DEVBENCH_BRIDGE_ENABLED

#	include "AssetReadProfiling.h"
#	include "ChangeFormProfiling.h"
#	include "Export.h"
#	include "LoadProfiling.h"

#	include <DevBenchAPI.h>
#	include <rapidjson/document.h>
#	include <rapidjson/stringbuffer.h>
#	include <rapidjson/writer.h>

#	include <atomic>
#	include <chrono>
#	include <future>
#	include <memory>
#	include <string>

namespace {
	using namespace rapidjson;

	// {saveLoads, changeFormsByMod, assetReads} as a JSON string -- same shape as the
	// JSON export. Reads the profiler snapshots; must run on the main thread (below).
	std::string BuildStatsJson() {
		Document doc(kObjectType);
		auto& alloc = doc.GetAllocator();

		Value loads(kArrayType);
		for (const auto& l : LoadProfiling::Snapshot()) {
			Value o(kObjectType);
			o.AddMember("name", Value(l.name.c_str(), alloc), alloc);
			o.AddMember("kind", Value(l.kind.c_str(), alloc), alloc);
			o.AddMember("success", l.success, alloc);
			o.AddMember("deserialize_ms", l.deserializeMs, alloc);
			o.AddMember("pre_form_ms", l.preFormMs, alloc);
			o.AddMember("change_forms_ms", l.formSpanMs, alloc);
			o.AddMember("global_data_ms", l.globalDataMs, alloc);
			o.AddMember("post_form_other_ms", l.postFormOtherMs, alloc);
			o.AddMember("papyrus_ms", l.papyrusMs, alloc);
			o.AddMember("menu_visible_ms", l.menuVisibleMs, alloc);
			o.AddMember("in_control_ms", l.inControlMs, alloc);
			o.AddMember("trailing_ms", l.postToCloseMs, alloc);
			loads.PushBack(o, alloc);
		}
		doc.AddMember("saveLoads", loads, alloc);

		Value cf(kArrayType);
		for (const auto& r : ChangeFormProfiling::SnapshotLast()) {
			Value o(kObjectType);
			o.AddMember("plugin", Value(r.plugin.c_str(), alloc), alloc);
			o.AddMember("forms", r.count, alloc);
			o.AddMember("total_ms", r.totalMs, alloc);
			cf.PushBack(o, alloc);
		}
		doc.AddMember("changeFormsByMod", cf, alloc);

		Value reads(kObjectType);
		const auto addSource = [&](const char* name, const AssetReadProfiling::Stats& s) {
			Value v(kObjectType);
			v.AddMember("calls", s.calls, alloc);
			v.AddMember("bytes", s.bytes, alloc);
			v.AddMember("ms", static_cast<double>(s.totalNs) / 1'000'000.0, alloc);
			reads.AddMember(StringRef(name), v, alloc);
		};
		addSource("bsa", AssetReadProfiling::SnapshotArchive());
		addSource("loose", AssetReadProfiling::SnapshotLoose());
		doc.AddMember("assetReads", reads, alloc);

		// Path to the full Perfetto-ready JSON export (the inline data above is a summary).
		const auto& jsonPath = Export::LastJsonPath();
		if (!jsonPath.empty())
			doc.AddMember("latestExport", Value(jsonPath.c_str(), alloc), alloc);

		StringBuffer sb;
		Writer<StringBuffer> w(sb);
		doc.Accept(w);
		return sb.GetString();
	}

	// Fired on the game thread when a load finalizes: publish a compact event so agents
	// and scenario steps can align to the load boundary (devbench's event-bus model).
	void OnLoadFinalized(const LoadProfiling::LoadRecord& l) {
		auto* dvb = DevBenchAPI::GetDevBenchInterface001();
		if (!dvb)
			return;
		Document doc(kObjectType);
		auto& alloc = doc.GetAllocator();
		doc.AddMember("name", Value(l.name.c_str(), alloc), alloc);
		doc.AddMember("kind", Value(l.kind.c_str(), alloc), alloc);
		doc.AddMember("success", l.success, alloc);
		doc.AddMember("deserialize_ms", l.deserializeMs, alloc);
		doc.AddMember("global_data_ms", l.globalDataMs, alloc);
		doc.AddMember("papyrus_ms", l.papyrusMs, alloc);
		doc.AddMember("in_control_ms", l.inControlMs, alloc);
		StringBuffer sb;
		Writer<StringBuffer> w(sb);
		doc.Accept(w);
		dvb->EmitEvent("loadprofiler.load", sb.GetString());
	}

	// Handlers run on devbench's listener thread; the snapshots + plugin-name resolution
	// (TESDataHandler) are game-state reads, so marshal to the main thread. Bounded so a
	// stalled main thread (mid-load) can't hang the listener.
	std::string BuildStatsOnMainThread() {
		auto* task = SKSE::GetTaskInterface();
		if (!task)
			return R"({"error":"SKSE task interface unavailable"})";
		auto prom = std::make_shared<std::promise<std::string>>();
		auto cancelled = std::make_shared<std::atomic<bool>>(false);
		auto fut = prom->get_future();
		task->AddTask([prom, cancelled]() {
			if (cancelled->load(std::memory_order_acquire))
				return;
			try {
				prom->set_value(BuildStatsJson());
			} catch (...) {
				prom->set_value(R"({"error":"failed to build stats on main thread"})");
			}
		});
		if (fut.wait_for(std::chrono::milliseconds(5000)) != std::future_status::ready) {
			cancelled->store(true, std::memory_order_release);
			return R"j({"error":"main thread did not run within 5000ms (mid-load?)"})j";
		}
		return fut.get();
	}

	void StatsHandler(void*, const char*, void* a_sink, DevBenchAPI::WriteFn a_write) {
		std::string out;
		try {
			out = BuildStatsOnMainThread();
		} catch (...) {
			out = R"({"error":"loadprofiler.stats handler failed"})";
		}
		a_write(a_sink, out.c_str());  // host owns the buffer; call exactly once
	}
}

namespace DevBenchBridge {
	void Install() {
		auto* dvb = DevBenchAPI::GetDevBenchInterface001();
		if (!dvb)
			return;  // devbench absent
		// RegisterToolExtension is a 1.5.0+ vtable slot; older hosts lack it (calling it would crash).
		if (dvb->GetBuildNumber() < 10500) {
			logger::info("DevBenchBridge: devbench build {} < 1.5.0; loadprofiler inspect kind not registered", dvb->GetBuildNumber());
			return;
		}
		// readOnly + real inputSchema so `inspect kind=extensions` documents it.
		static constexpr const char* statsDesc =
			R"j({"description":"Read-only save-load profiler stats (inspect kind=loadprofiler), the same data as the CSV/TXT/JSON exports and MCP UI. Returns {saveLoads:[{name,kind,success,deserialize_ms,pre_form_ms,change_forms_ms,global_data_ms,post_form_other_ms,papyrus_ms,menu_visible_ms,in_control_ms,trailing_ms}],changeFormsByMod:[{plugin,forms,total_ms}] for the last load,assetReads:{bsa,loose:{calls,bytes,ms}} for the last load,latestExport:path to the full Perfetto-ready JSON trace}. ms fields are -1 when that anchor pair was not observed. A loadprofiler.load event is emitted on each completed load.","readOnly":true,"inputSchema":{"type":"object","properties":{}}})j";
		dvb->RegisterToolExtension("inspect", "loadprofiler", statsDesc, &StatsHandler, nullptr);
		LoadProfiling::SetLoadFinalizedCallback(&OnLoadFinalized);  // emit loadprofiler.load on each load
		logger::info("DevBenchBridge: registered inspect kind=loadprofiler + loadprofiler.load event (devbench build {})", dvb->GetBuildNumber());
	}
}

#else

namespace DevBenchBridge {
	void Install() {}  // inert without the devbench-api (DEVBENCH_BRIDGE_ENABLED)
}

#endif
