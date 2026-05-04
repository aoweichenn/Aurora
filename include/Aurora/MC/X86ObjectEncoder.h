#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace aurora {

class MachineInstr;

class X86ObjectEncoder {
public:
    using SymbolResolver = std::function<size_t(const char*)>;
    using RelocationSink = std::function<void(uint64_t, size_t, uint32_t, int64_t)>;

    void encode(const MachineInstr& mi,
                std::vector<uint8_t>& out,
                uint64_t textBaseOffset = 0,
                const SymbolResolver& resolveSymbol = {},
                const RelocationSink& addRelocation = {}) const;
};

} // namespace aurora
