#pragma once

namespace MessagingProfilerBackend {
    std::vector<std::string_view> GetMessageTypeNames();
    std::vector<std::pair<std::string, std::array<double, SKSE::MessagingInterface::kTotal>>> GetAverageDurations();
    std::array<double, SKSE::MessagingInterface::kTotal> GetTotalsAvgMs();
}

namespace MessagingProfilerUI {
    struct State {
        std::vector<bool> selected;
        int sortColumn = 0;
        bool sortAsc = true;
        bool initializedFromDisk = false;
    };

    // Access global persistent state (single instance)
    State& GetState();

    void EnsureSelectionSize(State& s, std::size_t count);
    void Render(State& s, double warnMs, double critMs);

    // Persistence helpers
    void SetInitialVisibility(const std::vector<bool>& vis);
    std::vector<bool> GetCurrentVisibility();
}