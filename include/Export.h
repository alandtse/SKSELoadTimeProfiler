#pragma once

namespace Export {
    enum class Format {
        Csv = 0,
        Txt = 1
    };

    bool WriteSnapshot(Format format, std::string& statusMessage);
}