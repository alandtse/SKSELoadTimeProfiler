#include "Hooks.h"
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>
#include <psapi.h>

// --- Utility (kept from previous) -------------------------------------------------
size_t GetModuleSize(HMODULE module) {
    MODULEINFO moduleInfo;
    if (GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(moduleInfo))) {
        return moduleInfo.SizeOfImage;
    }
    return 0;
}

bool IsAddressInSkyrimExe(uintptr_t addr) {
    void* skyrimBase = GetModuleHandle(L"SkyrimSE.exe");
    if (!skyrimBase) return false;
    size_t skyrimSize = GetModuleSize(static_cast<HMODULE>(skyrimBase));
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(skyrimBase);
    return addr >= baseAddr && addr < (baseAddr + skyrimSize);
}

void PrintStackTrace() {
    void* skyrimSEBase = GetModuleHandle(L"SkyrimSE.exe");
    auto skyrimAddress = reinterpret_cast<uintptr_t>(skyrimSEBase);
    const int maxFrames = 64;
    void* stack[maxFrames];
    unsigned short frames = CaptureStackBackTrace(0, maxFrames, stack, NULL);
    for (unsigned short i = 0; i < frames; ++i) {
        auto currentAddress = reinterpret_cast<uintptr_t>(stack[i]);
        if (IsAddressInSkyrimExe(currentAddress)) {
            logger::critical("{:x}", 0x140000000 + currentAddress - skyrimAddress);
        }
    }
}

// --- ESP profiling aggregation ----------------------------------------------------
namespace ESPProfiling {
    struct Entry { std::string name; uint64_t totalNs{0}; uint64_t maxNs{0}; uint64_t count{0}; };
    static std::mutex g_mutex;
    static std::unordered_map<std::string, Entry> g_entries;

    void Record(std::string_view espName, uint64_t ns) {
        std::lock_guard lk(g_mutex);
        std::string key(espName);
        auto it = g_entries.find(key);
        if (it == g_entries.end()) it = g_entries.emplace(key, Entry{key,0,0,0}).first;
        auto& e = it->second;
        e.count++;
        e.totalNs += ns;
        if (ns > e.maxNs) e.maxNs = ns;
    }

    std::vector<std::pair<std::string, uint64_t>> SnapshotTotals() {
        std::lock_guard lk(g_mutex);
        std::vector<std::pair<std::string,uint64_t>> out; out.reserve(g_entries.size());
        for (auto& [k,v] : g_entries) out.emplace_back(k, v.totalNs);
        return out;
    }
}

// --- Common timing wrapper --------------------------------------------------------
namespace {
    template <class Fn, class... Args>
    auto TimeCall(const char* tag, const std::string& nameStr, Fn&& fn, Args&&... args) {
        const char* name = nameStr.empty() ? nullptr : nameStr.c_str();
        const auto start = std::chrono::high_resolution_clock::now();
        auto result = fn(std::forward<Args>(args)...);
        const auto end = std::chrono::high_resolution_clock::now();
        const auto ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        if (name) ESPProfiling::Record(name, ns);
        logger::info("[ESP] {} {} took {:.3f} ms", tag, name ? name : "<null>", ns / 1'000'000.0);
        return result;
    }
}

// --- Install entrypoint -----------------------------------------------------------
void Hooks::Install() {
    TESLoad::Install();
    OpenTESHook::Install();
    CloseTESHook::Install();
}

// --- TESLoad single hook ----------------------------------------------------------
void Hooks::TESLoad::Install() {
    auto& trampoline = SKSE::GetTrampoline();
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(13687, 13753).address() + REL::Relocate(0x5e, 0x323), thunk);
}

int64_t Hooks::TESLoad::thunk(int64_t a1, RE::TESFile* file, char a2) {
    auto fn = originalFunction.get();
    std::string filename;
    if (file) { auto sv = file->GetFilename(); filename.assign(sv.data(), sv.size()); } else { filename = "<null>"; }
    logger::info("TESLoad thunk ({})", filename);
    return TimeCall("Load", filename, fn, a1, file, a2);
}

// --- CloseTESHook multiple thunks -------------------------------------------------
void Hooks::CloseTESHook::Install() {
    auto& trampoline = SKSE::GetTrampoline();

    SKSE::AllocTrampoline(14); originalFunction1 = trampoline.write_call<5>(REL::RelocationID(13939, 14040).address() + REL::Relocate(0x2a1, 0x2b6), thunk1);
    SKSE::AllocTrampoline(14); originalFunction2 = trampoline.write_call<5>(REL::RelocationID(13651, 13759).address() + REL::Relocate(0x2f, 0x2f), thunk2);
    SKSE::AllocTrampoline(14); originalFunction3 = trampoline.write_call<5>(REL::RelocationID(13853, 13928).address() + REL::Relocate(0x9f, 0xa2), thunk3);
    SKSE::AllocTrampoline(14); originalFunction4 = trampoline.write_call<5>(REL::RelocationID(13652, 13760).address() + REL::Relocate(0x2a0, 0x371), thunk4);
    SKSE::AllocTrampoline(14); originalFunction5 = trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0x102, 0x101), thunk5);
    SKSE::AllocTrampoline(14); originalFunction6 = trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0x430, 0x110), thunk6);
    SKSE::AllocTrampoline(14); originalFunction7 = trampoline.write_call<5>(REL::RelocationID(13639, 13744).address() + REL::Relocate(0x1ac, 0x1b0), thunk7);
    SKSE::AllocTrampoline(14); originalFunction8 = trampoline.write_call<5>(REL::RelocationID(13749, 13861).address() + REL::Relocate(0x111, 0x114), thunk8);
}

bool Hooks::CloseTESHook::thunk1(RE::TESFile* file, bool a_force) { logger::info("Close thunk1 {}", file?file->GetFilename():"<null>"); return originalFunction1(file, a_force); }
bool Hooks::CloseTESHook::thunk2(RE::TESFile* file, bool a_force) { logger::info("Close thunk2 {}", file?file->GetFilename():"<null>"); return originalFunction2(file, a_force); }
bool Hooks::CloseTESHook::thunk3(RE::TESFile* file, bool a_force) { logger::info("Close thunk3 {}", file?file->GetFilename():"<null>"); return originalFunction3(file, a_force); }
bool Hooks::CloseTESHook::thunk4(RE::TESFile* file, bool a_force) { logger::info("Close thunk4 {}", file?file->GetFilename():"<null>"); return originalFunction4(file, a_force); }
bool Hooks::CloseTESHook::thunk5(RE::TESFile* file, bool a_force) { logger::info("Close thunk5 {}", file?file->GetFilename():"<null>"); return originalFunction5(file, a_force); }
bool Hooks::CloseTESHook::thunk6(RE::TESFile* file, bool a_force) { logger::info("Close thunk6 {}", file?file->GetFilename():"<null>"); return originalFunction6(file, a_force); }
bool Hooks::CloseTESHook::thunk7(RE::TESFile* file, bool a_force) { logger::info("Close thunk7 {}", file?file->GetFilename():"<null>"); return originalFunction7(file, a_force); }
bool Hooks::CloseTESHook::thunk8(RE::TESFile* file, bool a_force) { logger::info("Close thunk8 {}", file?file->GetFilename():"<null>"); return originalFunction8(file, a_force); }

// --- OpenTESHook multiple thunks --------------------------------------------------
void Hooks::OpenTESHook::Install() {
    auto& trampoline = SKSE::GetTrampoline();

    SKSE::AllocTrampoline(14); originalFunction1  = trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0x24b, 0x23b), thunk1);
    SKSE::AllocTrampoline(14); originalFunction2  = trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0x2ab, 0x28b), thunk2);
    SKSE::AllocTrampoline(14); originalFunction3  = trampoline.write_call<5>(REL::RelocationID(13652, 13760).address() + REL::Relocate(0x3c, 0x3c), thunk3);
    SKSE::AllocTrampoline(14); originalFunction4  = trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x45e, 0x3f0), thunk4);
    SKSE::AllocTrampoline(14); originalFunction5  = trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x60c, 0x5bc), thunk5);
    SKSE::AllocTrampoline(14); originalFunction6  = trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x802, 0x7a4), thunk6);
    SKSE::AllocTrampoline(14); originalFunction7  = trampoline.write_call<5>(REL::RelocationID(22500, 22975).address() + REL::Relocate(0x3c, 0x3e), thunk7);
    SKSE::AllocTrampoline(14); originalFunction8  = trampoline.write_call<5>(REL::RelocationID(22501, 22976).address() + REL::Relocate(0x3c, 0x3e), thunk8);
    SKSE::AllocTrampoline(14); originalFunction9  = trampoline.write_call<5>(REL::RelocationID(24266, 24780).address() + REL::Relocate(0x26, 0x26), thunk9);
    SKSE::AllocTrampoline(14); originalFunction10 = trampoline.write_call<5>(REL::RelocationID(24998, 25519).address() + REL::Relocate(0x93, 0x93), thunk10);
}

bool Hooks::OpenTESHook::thunk1 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk1 {}",  file?file->GetFilename():"<null>"); return originalFunction1(file,m,l); }
bool Hooks::OpenTESHook::thunk2 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk2 {}",  file?file->GetFilename():"<null>"); return originalFunction2(file,m,l); }
bool Hooks::OpenTESHook::thunk3 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk3 {}",  file?file->GetFilename():"<null>"); return originalFunction3(file,m,l); }
bool Hooks::OpenTESHook::thunk4 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk4 {}",  file?file->GetFilename():"<null>"); return originalFunction4(file,m,l); }
bool Hooks::OpenTESHook::thunk5 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk5 {}",  file?file->GetFilename():"<null>"); return originalFunction5(file,m,l); }
bool Hooks::OpenTESHook::thunk6 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk6 {}",  file?file->GetFilename():"<null>"); return originalFunction6(file,m,l); }
bool Hooks::OpenTESHook::thunk7 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk7 {}",  file?file->GetFilename():"<null>"); return originalFunction7(file,m,l); }
bool Hooks::OpenTESHook::thunk8 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk8 {}",  file?file->GetFilename():"<null>"); return originalFunction8(file,m,l); }
bool Hooks::OpenTESHook::thunk9 (RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk9 {}",  file?file->GetFilename():"<null>"); return originalFunction9(file,m,l); }
bool Hooks::OpenTESHook::thunk10(RE::TESFile* file, RE::NiFile::OpenMode m, bool l) { logger::info("Open thunk10 {}", file?file->GetFilename():"<null>"); return originalFunction10(file,m,l); }
