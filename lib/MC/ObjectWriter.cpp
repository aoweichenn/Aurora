#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/MC/X86/X86ObjectEncoder.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Constant.h"
#include <algorithm>
#include <fstream>
#include <cstring>
#include <stdexcept>

namespace aurora {

// ---- ELF64 structures ----
#pragma pack(push, 1)
struct Elf64_Ehdr {
    uint8_t ident[16];
    uint16_t type;       uint16_t machine;
    uint32_t version;
    uint64_t entry;      uint64_t phoff;     uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;     uint16_t phentsize; uint16_t phnum;     uint16_t shentsize;
    uint16_t shnum;      uint16_t shstrndx;
};

struct Elf64_Shdr {
    uint32_t name;       uint32_t type;
    uint64_t flags;      uint64_t addr;      uint64_t offset;    uint64_t size;
    uint32_t link;       uint32_t info;
    uint64_t addralign;  uint64_t entsize;
};

struct Elf64_Sym {
    uint32_t name;
    uint8_t info;        uint8_t other;
    uint16_t shndx;
    uint64_t value;      uint64_t size;
};

struct Elf64_Rela {
    uint64_t offset;     uint64_t info;
    int64_t addend;
};
#pragma pack(pop)

namespace {

unsigned byteSize(Type* type) {
    if (!type)
        return 8;
    return std::max(1u, (type->getSizeInBits() + 7u) / 8u);
}

void appendLittleEndian(std::vector<uint8_t>& out, uint64_t value, unsigned bytes) {
    for (unsigned index = 0; index < bytes; ++index) {
        out.push_back(static_cast<uint8_t>(value & 0xFFu));
        value >>= 8;
    }
}

void appendZeroDataForType(std::vector<uint8_t>& out, Type* type) {
    if (type && type->isArray()) {
        for (unsigned index = 0; index < type->getNumElements(); ++index)
            appendZeroDataForType(out, type->getElementType());
        return;
    }
    appendLittleEndian(out, 0, byteSize(type));
}

void appendConstantData(std::vector<uint8_t>& out, Constant* init, Type* type) {
    if (auto* array = dynamic_cast<ConstantArray*>(init)) {
        Type* elementType = type && type->isArray() ? type->getElementType() : nullptr;
        const size_t count = type && type->isArray() ? type->getNumElements() : array->getNumElements();
        for (size_t index = 0; index < count; ++index) {
            if (auto* element = array->getElement(index))
                appendConstantData(out, element, elementType);
            else
                appendZeroDataForType(out, elementType);
        }
        return;
    }
    if (auto* ci = dynamic_cast<ConstantInt*>(init)) {
        appendLittleEndian(out, ci->getZExtValue(), byteSize(type));
        return;
    }
    appendZeroDataForType(out, type);
}

} // namespace

ObjectWriter::ObjectWriter() : functionCount_(0) {
    // Add the null symbol (index 0)
    symbols_.push_back({"", 0, 0, 0, 0, 0});
}

size_t ObjectWriter::getOrCreateUndefinedSymbol(const std::string& name) {
    auto it = symbolIndexMap_.find(name);
    if (it != symbolIndexMap_.end()) return it->second;

    symbols_.push_back({name, 0, 0, 0, STB_GLOBAL, 0});
    const size_t symIdx = symbols_.size() - 1;
    symbolIndexMap_[name] = symIdx;
    return symIdx;
}

size_t ObjectWriter::defineSymbol(const std::string& name, uint64_t value, uint64_t size,
                                  uint8_t type, uint8_t bind, uint16_t shndx) {
    auto it = symbolIndexMap_.find(name);
    if (it != symbolIndexMap_.end()) {
        symbols_[it->second] = {name, value, size, type, bind, shndx};
        return it->second;
    }

    symbols_.push_back({name, value, size, type, bind, shndx});
    const size_t symIdx = symbols_.size() - 1;
    symbolIndexMap_[name] = symIdx;
    return symIdx;
}

void ObjectWriter::addFunction(MachineFunction& mf) {
    const auto& name = mf.getAIRFunction().getName();
    std::vector<uint8_t> funcBytes;
    std::map<std::string, size_t> labelOffsets; // label → offset in funcBytes
    std::vector<std::pair<size_t, std::string>> branchFixups; // (offset in funcBytes, target label)

    for (auto& mbb : mf.getBlocks()) {
        labelOffsets[mbb->getName()] = funcBytes.size();
        MachineInstr* mi = mbb->getFirst();
        while (mi) {
            uint16_t opc = mi->getOpcode();

            // Record current position
            size_t posBefore = funcBytes.size();

            // CALL to external symbol
            if (opc == X86::CALL64pcrel32 && mi->getNumOperands() >= 1 && mi->getOperand(0).getKind() == MachineOperandKind::MO_GlobalSym) {
                const char* callee = mi->getOperand(0).getGlobalSym();
                if (callee) {
                    const size_t symIdx = getOrCreateUndefinedSymbol(callee);
                    funcBytes.push_back(0xE8);
                    size_t relocOff = funcBytes.size();
                    for (int i = 0; i < 4; i++) funcBytes.push_back(0);
                    relocs_.push_back({textBytes_.size() + relocOff, symIdx, R_X86_64_PLT32, -4});
                }
                mi = mi->getNext(); continue;
            }

            // Check for JMP/Jcc with MBB target
            bool isBranch = (opc == X86::JMP_1 || opc == X86::JE_1 || opc == X86::JNE_1 ||
                             opc == X86::JL_1 || opc == X86::JG_1 || opc == X86::JLE_1 ||
                             opc == X86::JGE_1 || opc == X86::JAE_1 || opc == X86::JBE_1 ||
                             opc == X86::JA_1 || opc == X86::JB_1);
            std::string branchTarget;
            if (isBranch && mi->getNumOperands() >= 1 && mi->getOperand(0).getKind() == MachineOperandKind::MO_MBB) {
                branchTarget = mi->getOperand(0).getMBB()->getName();
            }

            // Encode instruction
            encodeInstruction(*mi, funcBytes);

            // Record branch fixup
            if (!branchTarget.empty())
                branchFixups.push_back({posBefore + 1, branchTarget}); // +1 = after opcode byte

            mi = mi->getNext();
        }
    }

    // Apply branch fixups
    for (auto& [off, target] : branchFixups) {
        auto it = labelOffsets.find(target);
        if (it == labelOffsets.end() || it->second == static_cast<size_t>(-1)) continue;
        int64_t rel = static_cast<int64_t>(it->second) - static_cast<int64_t>(off + 1);
        if (rel >= -128 && rel <= 127)
            funcBytes[off] = static_cast<uint8_t>(rel);
        else
            throw std::runtime_error("short branch target out of range");
    }

    defineSymbol(name, textBytes_.size(), funcBytes.size(), STT_FUNC, STB_GLOBAL, 1);
    textBytes_.insert(textBytes_.end(), funcBytes.begin(), funcBytes.end());
    functionCount_++;
}

void ObjectWriter::addGlobal(const GlobalVariable& gv) {
    size_t off = dataBytes_.size();
    appendConstantData(dataBytes_, gv.getInitializer(), gv.getType());
    defineSymbol(gv.getName(), off, dataBytes_.size() - off, STT_OBJECT, STB_GLOBAL, 2);
}

void ObjectWriter::addExternSymbol(const std::string& name) {
    getOrCreateUndefinedSymbol(name);
}

void ObjectWriter::encodeInstruction(const MachineInstr& mi, std::vector<uint8_t>& out) {
    X86ObjectEncoder encoder;
    auto resolveSymbol = [this](const char* sym) -> size_t {
        return getOrCreateUndefinedSymbol(sym);
    };
    auto addReloc = [this](uint64_t offset, size_t symIdx, uint32_t type, int64_t addend) {
        addRelocation(offset, symIdx, type, addend);
    };
    encoder.encode(mi, out, textBytes_.size(), resolveSymbol, addReloc);
}

void ObjectWriter::addRelocation(uint64_t offset, uint64_t symIdx, uint32_t type, int64_t addend) {
    relocs_.push_back({offset, symIdx, type, addend});
}

bool ObjectWriter::write(const std::string& path) {
    return writeELF(path);
}

bool ObjectWriter::writeELF(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    // Build section header string table
    std::string shstrtab;
    auto addShStr = [&](const std::string& s) -> uint32_t {
        uint32_t off = static_cast<uint32_t>(shstrtab.size());
        shstrtab += s + '\0';
        return off;
    };
    uint32_t sh_null = addShStr("");           // idx 0
    uint32_t sh_text = addShStr(".text");       // idx 1
    uint32_t sh_data = addShStr(".data");       // idx 2
    uint32_t sh_symtab = addShStr(".symtab");   // idx 3
    uint32_t sh_strtab = addShStr(".strtab");   // idx 4
    uint32_t sh_relatext = addShStr(".rela.text"); // idx 5
    uint32_t sh_shstrtab = addShStr(".shstrtab"); // idx 6

    // Build string table for symbols
    std::string strtab;
    auto addStr = [&](const std::string& s) -> uint32_t {
        uint32_t off = static_cast<uint32_t>(strtab.size());
        strtab += s + '\0';
        return off;
    };
    addStr(""); // null string at index 0
    std::vector<uint32_t> symNameOffs = {0};
    for (size_t i = 1; i < symbols_.size(); i++)
        symNameOffs.push_back(addStr(symbols_[i].name));

    // Compute sizes and offsets
    uint64_t elfHdrSize = 64;
    uint64_t shdrOff = elfHdrSize;
    uint64_t shdrEntrySize = 64;
    uint16_t shdrCount = 7;
    uint64_t shdrTotalSize = shdrCount * shdrEntrySize;

    auto alignTo = [](uint64_t value, uint64_t alignment) -> uint64_t {
        return alignment == 0 ? value : ((value + alignment - 1) / alignment) * alignment;
    };

    uint64_t textOff = alignTo(shdrOff + shdrTotalSize, 16);
    uint64_t dataOff = alignTo(textOff + textBytes_.size(), 8);
    uint64_t symtabOff = alignTo(dataOff + dataBytes_.size(), 8);
    uint64_t symtabSize = symbols_.size() * 24;
    uint64_t strtabOff = symtabOff + symtabSize;
    uint64_t relatextOff = alignTo(strtabOff + strtab.size(), 8);
    uint64_t relatextSize = relocs_.size() * 24;
    uint64_t shstrtabOff = relatextOff + relatextSize;

    // ---- ELF Header ----
    Elf64_Ehdr ehdr = {};
    memset(ehdr.ident, 0, 16);
    ehdr.ident[0] = 0x7F; ehdr.ident[1] = 'E'; ehdr.ident[2] = 'L'; ehdr.ident[3] = 'F';
    ehdr.ident[4] = 2;  // 64-bit
    ehdr.ident[5] = 1;  // little-endian
    ehdr.ident[6] = 1;  // ELF version
    ehdr.type = 1;      // ET_REL
    ehdr.machine = 62;  // EM_X86_64
    ehdr.version = 1;
    ehdr.shoff = shdrOff;
    ehdr.ehsize = 64;
    ehdr.shentsize = 64;
    ehdr.shnum = shdrCount;
    ehdr.shstrndx = 6;

    file.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));

    // ---- Section Headers ----
    Elf64_Shdr shdrs[7] = {};
    auto setShdr = [&](int idx, uint32_t name, uint32_t type, uint64_t flags, uint64_t addr,
                        uint64_t off, uint64_t size, uint32_t link, uint32_t info, uint64_t align, uint64_t entsize) {
        shdrs[idx].name = name; shdrs[idx].type = type; shdrs[idx].flags = flags;
        shdrs[idx].addr = addr; shdrs[idx].offset = off; shdrs[idx].size = size;
        shdrs[idx].link = link; shdrs[idx].info = info;
        shdrs[idx].addralign = align; shdrs[idx].entsize = entsize;
    };

    // 0: NULL
    setShdr(0, sh_null, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // 1: .text
    setShdr(1, sh_text, 1, 6, 0, textOff, textBytes_.size(), 0, 0, 16, 0);
    // 2: .data
    setShdr(2, sh_data, 1, 3, 0, dataOff, dataBytes_.size(), 0, 0, 8, 0);
    // 3: .symtab
    setShdr(3, sh_symtab, 2, 0, 0, symtabOff, symtabSize, 4, 1, 8, 24);
    // 4: .strtab
    setShdr(4, sh_strtab, 3, 0, 0, strtabOff, strtab.size(), 0, 0, 1, 0);
    // 5: .rela.text
    setShdr(5, sh_relatext, 4, 0, 0, relatextOff, relatextSize, 3, 1, 8, 24);
    // 6: .shstrtab
    setShdr(6, sh_shstrtab, 3, 0, 0, shstrtabOff, shstrtab.size(), 0, 0, 1, 0);

    // Write section headers
    for (int i = 0; i < shdrCount; i++)
        file.write(reinterpret_cast<const char*>(&shdrs[i]), sizeof(Elf64_Shdr));

    auto padTo = [&](uint64_t offset) {
        auto current = static_cast<uint64_t>(file.tellp());
        while (current < offset) {
            file.put('\0');
            ++current;
        }
    };

    // ---- .text section ----
    padTo(textOff);
    file.write(reinterpret_cast<const char*>(textBytes_.data()), static_cast<std::streamsize>(textBytes_.size()));


    // ---- .data section ----
    padTo(dataOff);
    file.write(reinterpret_cast<const char*>(dataBytes_.data()), static_cast<std::streamsize>(dataBytes_.size()));

    // ---- .symtab section ----
    padTo(symtabOff);
    for (size_t i = 0; i < symbols_.size(); i++) {
        Elf64_Sym sym = {};
        sym.name = symNameOffs[i];
        sym.info = static_cast<uint8_t>((symbols_[i].bind << 4) | (symbols_[i].type & 0xF));
        sym.other = STV_DEFAULT;
        sym.shndx = symbols_[i].shndx;
        sym.value = symbols_[i].value;
        sym.size = symbols_[i].size;
        file.write(reinterpret_cast<const char*>(&sym), sizeof(sym));
    }

    // ---- .strtab section ----
    padTo(strtabOff);
    file.write(strtab.data(), static_cast<std::streamsize>(strtab.size()));


    // ---- .rela.text section ----
    padTo(relatextOff);
    for (auto& r : relocs_) {
        Elf64_Rela rela = {};
        rela.offset = r.offset;
        rela.info = (r.symbolIndex << 32) | r.type;
        rela.addend = r.addend;
        file.write(reinterpret_cast<const char*>(&rela), sizeof(rela));
    }

    // ---- .shstrtab section ----
    padTo(shstrtabOff);
    file.write(shstrtab.data(), static_cast<std::streamsize>(shstrtab.size()));
    return file.good();
}

} // namespace aurora
