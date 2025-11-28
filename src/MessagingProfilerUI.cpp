#include "MessagingProfilerUI.h"
#include "MCP.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>
#include <algorithm>
#include <unordered_map>
#include <windows.h>
#include "Settings.h"

namespace MessagingProfilerBackend {
    std::vector<std::string_view> GetMessageTypeNames();
    std::vector<std::pair<std::string, std::array<double, SKSE::MessagingInterface::kTotal>>> GetAverageDurations();

    enum class SourceKind { DLL, ESP };

    struct TaggedRow {
        std::string module;
        SourceKind kind;
        double totalMs;
        std::array<double, SKSE::MessagingInterface::kTotal> perMsg{};
    };

    std::vector<TaggedRow> GetTaggedRows();
}

namespace MessagingProfilerUI {
    static State g_state; // singleton
    State& GetState() { return g_state; }

    void EnsureSelectionSize(State& s, const std::size_t count) {
        if (s.selected.size() != count) s.selected.assign(count, true);
    }

    static void ColorCell(const double v, const double warnMs, const double critMs) {
        ImGuiMCP::ImU32 col = 0;
        if (v >= critMs)
            col = ImGuiMCP::ImGui::GetColorU32(ImGuiMCP::ImVec4(0.85f, 0.15f, 0.15f, 0.50f));
        else if (v >= warnMs) col = ImGuiMCP::ImGui::GetColorU32(ImGuiMCP::ImVec4(0.95f, 0.75f, 0.10f, 0.40f));
        if (col) ImGuiMCP::ImGui::TableSetBgColor(ImGuiMCP::ImGuiTableBgTarget_CellBg, col);
    }

    void SetInitialVisibility(const std::vector<bool>& vis) {
        g_state.selected = vis;
        g_state.initializedFromDisk = true;
    }

    std::vector<bool> GetCurrentVisibility() { return g_state.selected; }

    struct DllMeta {
        std::string author;
        std::string version;
        std::string license;
        bool ok = false;
    };

    static std::unordered_map<std::string, DllMeta> g_metaCache;

    static bool QueryVersionString(const wchar_t* path, const wchar_t* key, std::wstring& out) {
        DWORD handle = 0;
        DWORD sz = GetFileVersionInfoSizeW(path, &handle);
        if (!sz) return false;
        std::vector<char> buf(sz);
        if (!GetFileVersionInfoW(path, 0, sz, buf.data())) return false;
        struct LANGANDCODEPAGE {
            WORD wLanguage;
            WORD wCodePage;
        };
        LANGANDCODEPAGE* lpTranslate = nullptr;
        UINT cbTranslate = 0;
        if (!VerQueryValueW(buf.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate) ||
            cbTranslate < sizeof(LANGANDCODEPAGE)) {
            static LANGANDCODEPAGE fallback{0x0409, 1200};
            lpTranslate = &fallback;
            cbTranslate = sizeof(LANGANDCODEPAGE);
        }
        wchar_t query[256]{};
        for (UINT i = 0; i < cbTranslate / sizeof(LANGANDCODEPAGE); ++i) {
            _snwprintf_s(query,_TRUNCATE, L"\\StringFileInfo\\%04x%04x\\%s", lpTranslate[i].wLanguage,
                         lpTranslate[i].wCodePage, key);
            LPVOID val = nullptr;
            UINT bytes = 0;
            if (VerQueryValueW(buf.data(), query, &val, &bytes) && val && bytes) {
                out.assign(static_cast<const wchar_t*>(val));
                return true;
            }
        }
        return false;
    }

    static std::string WideToUtf8(const std::wstring& ws) {
        if (ws.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};
        std::string out(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len, nullptr, nullptr);
        return out;
    }

    static DllMeta GetDllMeta(const std::string& moduleBase) {
        DllMeta meta;
        std::string dllName = moduleBase + ".dll";
        HMODULE h = GetModuleHandleA(dllName.c_str());
        if (!h) return meta;
        wchar_t wpath[MAX_PATH]{};
        if (!GetModuleFileNameW(h, wpath,MAX_PATH)) return meta;
        std::wstring w;
        if (QueryVersionString(wpath, L"CompanyName", w) || QueryVersionString(wpath, L"Author", w))
            meta.author = WideToUtf8(w);
        if (QueryVersionString(wpath, L"ProductVersion", w) || QueryVersionString(wpath, L"FileVersion", w))
            meta.version = WideToUtf8(w);
        if (QueryVersionString(wpath, L"License", w) || QueryVersionString(wpath, L"LegalCopyright", w))
            meta.license = WideToUtf8(w);
        meta.ok = !(meta.author.empty() && meta.version.empty() && meta.license.empty());
        return meta;
    }

    void Render(State& s, const double warnMs, const double critMs) {
        auto names = MessagingProfilerBackend::GetMessageTypeNames();
        EnsureSelectionSize(s, names.size());
        // If not initialized from disk and first frame (all true already), ensure default all true explicitly.
        if (!s.initializedFromDisk && !s.selected.empty()) {
            bool any = false;
            for (bool b : s.selected) {
                if (b) {
                    any = true;
                    break;
                }
            }
            if (!any) std::fill(s.selected.begin(), s.selected.end(), true);
        }

        auto taggedRows = MessagingProfilerBackend::GetTaggedRows();

        if (ImGuiMCP::ImGui::CollapsingHeader("Message Types")) {
            ImGuiMCP::ImGui::Indent();
            for (std::size_t i = 0; i < names.size(); ++i) {
                ImGuiMCP::ImGui::PushID(static_cast<int>(i));
                bool sel = s.selected[i];
                if (ImGuiMCP::ImGui::Checkbox(names[i].data(), &sel)) {
                    s.selected[i] = sel;
                    LogSettings::Save();
                }
                ImGuiMCP::ImGui::PopID();
                if ((i % 4) != 3) ImGuiMCP::ImGui::SameLine();
            }
            if (ImGuiMCP::ImGui::Button("All")) {
                std::fill(s.selected.begin(), s.selected.end(), true);
                LogSettings::Save();
            }
            ImGuiMCP::ImGui::SameLine();
            if (ImGuiMCP::ImGui::Button("None")) {
                std::fill(s.selected.begin(), s.selected.end(), false);
                LogSettings::Save();
            }
            ImGuiMCP::ImGui::Unindent();
        }

        std::vector<std::size_t> active;
        for (std::size_t i = 0; i < s.selected.size(); ++i) if (s.selected[i]) active.push_back(i);
        if (active.empty()) {
            ImGuiMCP::ImGui::TextUnformatted("No message types selected.");
            return;
        }

        // Apply source filters from MCP
        std::erase_if(taggedRows, [](const MessagingProfilerBackend::TaggedRow& r) {
            if (r.kind == MessagingProfilerBackend::SourceKind::DLL && !MCP::showDllEntries) return true;
            if (r.kind == MessagingProfilerBackend::SourceKind::ESP && !MCP::showEspEntries) return true;
            return false;
        });

        if (ImGuiMCP::ImGui::BeginTable("##msgprof2", static_cast<int>(active.size()) + 3,
                                        ImGuiMCP::ImGuiTableFlags_RowBg | ImGuiMCP::ImGuiTableFlags_Borders |
                                        ImGuiMCP::ImGuiTableFlags_Resizable |
                                        ImGuiMCP::ImGuiTableFlags_Reorderable |
                                        ImGuiMCP::ImGuiTableFlags_Sortable | ImGuiMCP::ImGuiTableFlags_ScrollX |
                                        ImGuiMCP::ImGuiTableFlags_ScrollY)) {
            ImGuiMCP::ImGui::TableSetupColumn(
                "Module", ImGuiMCP::ImGuiTableColumnFlags_DefaultSort | ImGuiMCP::ImGuiTableColumnFlags_WidthStretch);
            ImGuiMCP::ImGui::TableSetupColumn("Type", ImGuiMCP::ImGuiTableColumnFlags_PreferSortDescending);
            ImGuiMCP::ImGui::TableSetupColumn("Total", ImGuiMCP::ImGuiTableColumnFlags_PreferSortDescending);
            for (auto idx : active)
                ImGuiMCP::ImGui::TableSetupColumn(names[idx].data(),
                                                  ImGuiMCP::ImGuiTableColumnFlags_PreferSortDescending);
            ImGuiMCP::ImGui::TableHeadersRow();
            if (ImGuiMCP::ImGuiTableSortSpecs* sortSpecs = ImGuiMCP::ImGui::TableGetSortSpecs())
                if (sortSpecs->SpecsCount > 0) {
                    s.sortColumn = sortSpecs->Specs[0].ColumnIndex;
                    s.sortAsc = sortSpecs->Specs[0].SortDirection == ImGuiMCP::ImGuiSortDirection_Ascending;
                }
            struct RowWrap {
                std::string module;
                std::string typeStr;
                double total;
                const std::array<double, SKSE::MessagingInterface::kTotal>* vals;
                bool isEsp;
            };
            std::vector<RowWrap> enriched;
            enriched.reserve(taggedRows.size());
            for (auto& r : taggedRows) {
                double sum = 0.0;
                if (r.kind == MessagingProfilerBackend::SourceKind::ESP) { sum = r.totalMs; } else {
                    for (auto idx : active) {
                        double v = r.perMsg[idx];
                        if (v < 1.0) v = 0.0;
                        sum += v;
                    }
                }
                enriched.push_back({r.module, r.kind == MessagingProfilerBackend::SourceKind::ESP ? "ESP" : "DLL", sum,
                                    &r.perMsg, r.kind == MessagingProfilerBackend::SourceKind::ESP});
            }
            std::ranges::sort(enriched, [&](const RowWrap& A, const RowWrap& B) {
                if (s.sortColumn == 0) return s.sortAsc ? A.module < B.module : A.module > B.module;
                if (s.sortColumn == 1) return s.sortAsc
                                                  ? A.typeStr < B.typeStr
                                                  : A.typeStr > B.typeStr;
                if (s.sortColumn == 2) return s.sortAsc ? A.total < B.total : A.total > B.total;
                std::size_t msgIdx = active[s.sortColumn - 3];
                double av = (*A.vals)[msgIdx];
                if (av < 1.0 || A.isEsp) av = 0.0;
                double bv = (*B.vals)[msgIdx];
                if (bv < 1.0 || B.isEsp) bv = 0.0;
                return s.sortAsc ? av < bv : av > bv;
            });
            std::vector<double> colTotals(active.size(), 0.0);
            for (auto& e : enriched) for (std::size_t c = 0; c < active.size(); ++c) {
                double v = (*e.vals)[active[c]];
                if (e.isEsp || v < 1.0) v = 0.0;
                colTotals[c] += v;
            }
            double totalsSum = 0.0;
            for (double v : colTotals) totalsSum += v;
            double espExtra = 0.0;
            for (auto& e : enriched) if (e.isEsp) espExtra += e.total;
            double grandTotal = totalsSum + espExtra;
            ImGuiMCP::ImGui::TableNextRow();
            ImGuiMCP::ImGui::TableSetColumnIndex(0);
            ImGuiMCP::ImGui::TextUnformatted("<Totals>");
            ImGuiMCP::ImGui::TableSetColumnIndex(1);
            ImGuiMCP::ImGui::TextUnformatted("-");
            ImGuiMCP::ImGui::TableSetColumnIndex(2);
            {
                ColorCell(grandTotal, warnMs, critMs);
                ImGuiMCP::ImGui::Text("%.1f", grandTotal);
            }
            for (std::size_t c = 0; c < active.size(); ++c) {
                ImGuiMCP::ImGui::TableSetColumnIndex(static_cast<int>(c + 3));
                double v = colTotals[c];
                ColorCell(v, warnMs, critMs);
                ImGuiMCP::ImGui::Text("%.1f", v);
            }
            for (auto& e : enriched) {
                ImGuiMCP::ImGui::TableNextRow();
                ImGuiMCP::ImGui::TableSetColumnIndex(0);
                ImGuiMCP::ImGui::TextUnformatted(e.module.c_str());
                if (!e.isEsp && ImGuiMCP::ImGui::IsItemHovered()) {
                    auto it = g_metaCache.find(e.module);
                    if (it == g_metaCache.end()) it = g_metaCache.emplace(e.module, GetDllMeta(e.module)).first;
                    if (ImGuiMCP::ImGui::BeginTooltip()) {
                        ImGuiMCP::ImGui::TextUnformatted(e.module.c_str());
                        if (it->second.ok) {
                            if (!it->second.author.empty()) ImGuiMCP::ImGui::Text(
                                "Author: %s", it->second.author.c_str());
                            if (!it->second.version.empty()) ImGuiMCP::ImGui::Text(
                                "Version: %s", it->second.version.c_str());
                            if (!it->second.license.empty()) ImGuiMCP::ImGui::Text(
                                "License: %s", it->second.license.c_str());
                        } else { ImGuiMCP::ImGui::TextUnformatted("(no version info)"); }
                        ImGuiMCP::ImGui::EndTooltip();
                    }
                }
                ImGuiMCP::ImGui::TableSetColumnIndex(1);
                ImGuiMCP::ImGui::TextUnformatted(e.typeStr.c_str());
                ImGuiMCP::ImGui::TableSetColumnIndex(2);
                ColorCell(e.total, warnMs, critMs);
                ImGuiMCP::ImGui::Text("%.1f", e.total);
                for (std::size_t c = 0; c < active.size(); ++c) {
                    ImGuiMCP::ImGui::TableSetColumnIndex(static_cast<int>(c + 3));
                    double v = (*e.vals)[active[c]];
                    if (v < 1.0 || e.isEsp) v = 0.0;
                    if (!e.isEsp) ColorCell(v, warnMs, critMs);
                    ImGuiMCP::ImGui::Text("%.1f", v);
                }
            }
            ImGuiMCP::ImGui::EndTable();
        }
    }
}