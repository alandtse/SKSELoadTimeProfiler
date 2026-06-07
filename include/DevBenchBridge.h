#pragma once

// Optional integration with the devbench test bench: registers a read-only
// `inspect kind=loadprofiler` extension (devbench 1.5.0+) exposing the same save-load
// data as the exports/UI. Compiles to a no-op without the devbench-api
// (DEVBENCH_BRIDGE_ENABLED), and is inert when no devbench host is present or it is < 1.5.0.
namespace DevBenchBridge {
    void Install();
}
