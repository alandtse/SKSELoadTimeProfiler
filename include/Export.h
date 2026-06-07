#pragma once

namespace Export {
    
    constexpr std::size_t kMaxExportFiles = 20;

    enum class Format {
        Csv = 0,
        Txt = 1,
        Json = 2,
        kTotal
    };

    bool WriteSnapshot(Format format, std::string& statusMessage);

    // Path of the most recently written JSON snapshot (empty until one is written).
    // Lets consumers point at the full Perfetto-ready trace, not just the summary.
    const std::string& LastJsonPath();
}