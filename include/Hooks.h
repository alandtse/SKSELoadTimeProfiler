#pragma once

namespace Hooks {
	void Install();

	class TESLoad {
    public:
        static void Install();

    private:
        static int64_t thunk(int64_t a1, RE::TESFile* file, char a2);
        static inline REL::Relocation<decltype(thunk)> originalFunction;
    };

    	class OpenTESHook {
    public:
        static void Install();

    private:
        static bool thunk(RE::TESFile* file, RE::NiFile::OpenMode a_accessMode, bool a_lock);
        static inline REL::Relocation<decltype(thunk)> originalFunction;
    };

    class CloseTESHook {
        public:
            static void Install();

        private:
            static bool thunk(RE::TESFile* file, bool a_force);
            static inline REL::Relocation<decltype(thunk)> originalFunction;
        };
};