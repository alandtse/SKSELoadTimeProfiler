#include "Hooks.h"

#include <psapi.h>
#include <windows.h>

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

void Hooks::Install() { 
    TESLoad::Install(); 
    OpenTESHook::Install();
    CloseTESHook::Install();

}

void Hooks::TESLoad::Install() {
    auto& trampoline = SKSE::GetTrampoline();

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0, 0x323), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0, 0x343), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x961, 0x9a7), thunk);
}

int64_t Hooks::TESLoad::thunk(int64_t a1, RE::TESFile* file, char a2) {
    logger::critical("TESTLoad Loading {}", file->GetFilename());
    auto result = originalFunction(a1, file, a2);
    logger::critical("TESLoad Finished {}", file->GetFilename());
    return result;
}

void Hooks::CloseTESHook::Install() {
    auto& trampoline = SKSE::GetTrampoline();

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13939, 14040).address() + REL::Relocate(0x2a1, 0x2b6), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13651, 13759).address() + REL::Relocate(0x2f, 0x2f), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13853, 13928).address() + REL::Relocate(0x9f, 0xa2), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(13652, 13760).address() + REL::Relocate(0x2a0, 0x371),
                                                thunk);  // this is the one from the load function
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(0, 441548).address() + REL::Relocate(0, 0x1b1), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 13846).address() + REL::Relocate(0, 0x2bd), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0x102, 0x101), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0x430, 0x110), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0, 0x120), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13638, 13743).address() + REL::Relocate(0, 0x440), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13639, 13744).address() + REL::Relocate(0x1ac, 0x1b0), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(0, 441527).address() + REL::Relocate(0, 0x3d4), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13749, 13861).address() + REL::Relocate(0x111, 0x114), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13854, 13930).address() + REL::Relocate(0, 0x256), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13854, 13930).address() + REL::Relocate(0, 0x301), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13858, 13934).address() + REL::Relocate(0, 0x4b), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 441556).address() + REL::Relocate(0, 0x50), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 441594).address() + REL::Relocate(0, 0x9d), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 418885).address() + REL::Relocate(0, 0x50), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 36344).address() + REL::Relocate(0, 0x14a), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(0, 442473).address() + REL::Relocate(0, 0x212), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 442475).address() + REL::Relocate(0, 0xa1), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(0, 442474).address() + REL::Relocate(0, 0x1f0), thunk);
    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 36345).address() + REL::Relocate(0, 0x17e), thunk);
}


bool Hooks::CloseTESHook::thunk(RE::TESFile* file, bool a_force) { 
    logger::critical("Close Finished{}", file->GetFilename());
    return originalFunction(file, a_force); }

void Hooks::OpenTESHook::Install() {
    auto& trampoline = SKSE::GetTrampoline();

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0x24b, 0x23b), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13645, 13753).address() + REL::Relocate(0x2ab, 0x28b), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13652, 13760).address() + REL::Relocate(0x3c, 0x3c), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction = trampoline.write_call<5>(REL::RelocationID(0, 13846).address() + REL::Relocate(0, 0x1f), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(0, 441527).address() + REL::Relocate(0, 0x1ec), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x45e, 0x3f0), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x60c, 0x5bc), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(13672, 13785).address() + REL::Relocate(0x802, 0x7a4), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(22500, 22975).address() + REL::Relocate(0x3c, 0x3e), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(22501, 22976).address() + REL::Relocate(0x3c, 0x3e), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(24266, 24780).address() + REL::Relocate(0x26, 0x26), thunk);

    SKSE::AllocTrampoline(14);
    originalFunction =
        trampoline.write_call<5>(REL::RelocationID(24998, 25519).address() + REL::Relocate(0x93, 0x93), thunk);
}

bool Hooks::OpenTESHook::thunk(RE::TESFile* file, RE::NiFile::OpenMode a_accessMode, bool a_lock) { 
    logger::critical("OPEN Loading {}", file->GetFilename());
    //PrintStackTrace();
    return originalFunction(file, a_accessMode, a_lock); 
}
