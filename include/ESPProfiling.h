#pragma once

namespace ESPProfiling {
    struct Entry {
        std::string name;
        std::string author;
        double version{-1.0};
        uint64_t totalNs{0};
        uint64_t maxNs{0};
        uint64_t count{0};
    };

    std::vector<Entry> SnapshotEntries();
    std::vector<std::pair<std::string, uint64_t>> SnapshotTotals();

    void Record(std::string_view espName, uint64_t ns, std::string_view author = {}, double version = -1.0);
    void SetCurrentLoading(std::string_view espName);
    void ClearCurrentLoading();
    std::string GetCurrentLoading();
    double GetCurrentLoadingElapsedMs();
}
