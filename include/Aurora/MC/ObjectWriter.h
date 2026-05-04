#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace aurora {

class MachineInstr;
class MachineFunction;
class MachineOperand;
class Module;
class GlobalVariable;
class X86ObjectEncoder;

struct RelocEntry {
    uint64_t offset;       // offset in .text
    uint64_t symbolIndex;  // index in .symtab
    uint32_t type;         // R_X86_64_PC32 / R_X86_64_PLT32
    int64_t addend;        // addend for RELA
};

const uint32_t R_X86_64_PC32 = 2;
const uint32_t R_X86_64_PLT32 = 4;
const uint32_t R_X86_64_32S = 11;

class ObjectWriter {
public:
    ObjectWriter();

    void addFunction(MachineFunction& mf);
    void addGlobal(const GlobalVariable& gv);
    void addExternSymbol(const std::string& name);

    bool write(const std::string& path);

private:
    struct Symbol {
        std::string name;
        uint64_t value;
        uint64_t size;
        uint8_t type;      // STT_FUNC / STT_OBJECT
        uint8_t bind;      // STB_LOCAL / STB_GLOBAL
        uint16_t shndx;    // section index
    };

    std::vector<uint8_t> textBytes_;
    std::vector<uint8_t> dataBytes_;
    std::vector<Symbol> symbols_;
    std::vector<RelocEntry> relocs_;
    std::map<std::string, size_t> symbolIndexMap_;
    std::vector<std::string> externSymbols_;

    size_t functionCount_ = 0;

    static constexpr uint8_t STT_FUNC = 2;
    static constexpr uint8_t STT_OBJECT = 1;
    static constexpr uint8_t STB_LOCAL = 0;
    static constexpr uint8_t STB_GLOBAL = 1;
    static constexpr uint8_t STV_DEFAULT = 0;

    void encodeInstruction(const MachineInstr& mi, std::vector<uint8_t>& out);
    void addRelocation(uint64_t offset, uint64_t symIdx, uint32_t type, int64_t addend);
    bool writeELF(const std::string& path);
};

} // namespace aurora
