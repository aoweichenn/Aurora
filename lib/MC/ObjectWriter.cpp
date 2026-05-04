#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Constant.h"
#include <fstream>
#include <cstring>

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

ObjectWriter::ObjectWriter() : functionCount_(0), funcOffset_(0) {
    // Add the null symbol (index 0)
    symbols_.push_back({"", 0, 0, 0, 0, 0});
}

void ObjectWriter::addFunction(MachineFunction& mf) {
    const auto& name = mf.getAIRFunction().getName();

    // Encode all instructions into textBytes_
    std::vector<uint8_t> funcBytes;
    for (auto& mbb : mf.getBlocks()) {
        MachineInstr* mi = mbb->getFirst();
        while (mi) {
            uint16_t opc = mi->getOpcode();

            // Check for CALL to external symbol
            if (opc == X86::CALL64pcrel32 && mi->getNumOperands() >= 1) {
                auto& mo = mi->getOperand(0);
                if (mo.getKind() == MachineOperandKind::MO_GlobalSym) {
                    const char* callee = mo.getGlobalSym();
                    if (callee) {
                        size_t symIdx = 0;
                        auto it = symbolIndexMap_.find(callee);
                        if (it == symbolIndexMap_.end()) {
                            symbols_.push_back({callee, 0, 0, 0, STB_GLOBAL, 0});
                            symIdx = symbols_.size() - 1;
                            symbolIndexMap_[callee] = symIdx;
                        } else {
                            symIdx = it->second;
                        }
                        // CALL rel32: encode E8 with 4 zero bytes, then add relocation
                        funcBytes.push_back(0xE8);
                        size_t relocOff = funcBytes.size();
                        for (int i = 0; i < 4; i++) funcBytes.push_back(0);
                        relocs_.push_back({textBytes_.size() + relocOff, symIdx, R_X86_64_PLT32, -4});
                    }
                }
                mi = mi->getNext();
                continue;
            }

            // Check for MOV64ri32 with global symbol (load address)
            if (opc == X86::MOV64ri32 && mi->getNumOperands() >= 1) {
                auto& mo = mi->getOperand(0);
                if (mo.getKind() == MachineOperandKind::MO_GlobalSym) {
                    const char* gsym = mo.getGlobalSym();
                    if (gsym) {
                        size_t symIdx = 0;
                        auto it = symbolIndexMap_.find(gsym);
                        if (it == symbolIndexMap_.end()) {
                            symbols_.push_back({gsym, 0, 0, STT_OBJECT, STB_GLOBAL, 2}); // section 2 = .data
                            symIdx = symbols_.size() - 1;
                            symbolIndexMap_[gsym] = symIdx;
                        } else {
                            symIdx = it->second;
                        }
                        // MOV r64, imm32 with REX.W: 0x48 0xC7 /0 + modrm + imm32
                        funcBytes.push_back(0x48);
                        funcBytes.push_back(0xC7);
                        // Determine destination reg from operand[1]
                        uint8_t dstReg = 0; // RAX default
                        if (mi->getNumOperands() >= 2 && mi->getOperand(1).isReg())
                            dstReg = mi->getOperand(1).getReg() & 7;
                        funcBytes.push_back(0xC0 | dstReg); // ModRM: mod=11, reg=0, rm=reg
                        size_t relocOff = funcBytes.size();
                        for (int i = 0; i < 4; i++) funcBytes.push_back(0);
                        relocs_.push_back({textBytes_.size() + relocOff, symIdx, R_X86_64_32S, 0});
                    }
                }
                mi = mi->getNext();
                continue;
            }

            // Default: encode instruction
            encodeInstruction(*mi, funcBytes);
            mi = mi->getNext();
        }
    }

    // Record symbol
    symbols_.push_back({name, textBytes_.size(), funcBytes.size(), STT_FUNC, STB_GLOBAL, 1});
    symbolIndexMap_[name] = symbols_.size() - 1;

    textBytes_.insert(textBytes_.end(), funcBytes.begin(), funcBytes.end());
    functionCount_++;
}

void ObjectWriter::addGlobal(const GlobalVariable& gv) {
    uint64_t val = 0;
    if (auto* init = gv.getInitializer()) {
        if (auto* ci = dynamic_cast<const ConstantInt*>(init))
            val = ci->getZExtValue();
    }
    size_t off = dataBytes_.size();
    for (int i = 0; i < (gv.getType()->getSizeInBits() / 8); i++) {
        dataBytes_.push_back(static_cast<uint8_t>(val & 0xFF));
        val >>= 8;
    }
    symbols_.push_back({gv.getName(), off, gv.getType()->getSizeInBits() / 8, STT_OBJECT, STB_GLOBAL, 2});
    symbolIndexMap_[gv.getName()] = symbols_.size() - 1;
}

void ObjectWriter::addExternSymbol(const std::string& name) {
    auto it = symbolIndexMap_.find(name);
    if (it == symbolIndexMap_.end()) {
        symbols_.push_back({name, 0, 0, 0, STB_GLOBAL, 0});
        symbolIndexMap_[name] = symbols_.size() - 1;
    }
}

void ObjectWriter::encodeInstruction(MachineInstr& mi, std::vector<uint8_t>& out) {
    uint16_t opc = mi.getOpcode();
    unsigned numOps = mi.getNumOperands();

    // x86 opcode lookup table — simplified for common instructions
    auto emitModRM = [&](uint8_t mod, uint8_t reg, uint8_t rm) {
        out.push_back((mod << 6) | ((reg & 7) << 3) | (rm & 7));
    };
    auto emitImm32 = [&](int64_t v) {
        for (int i = 0; i < 4; i++) out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
    };
    auto emitImm64 = [&](int64_t v) {
        for (int i = 0; i < 8; i++) out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
    };
    auto getReg = [&](unsigned idx) -> uint8_t {
        if (idx < numOps && mi.getOperand(idx).isReg())
            return mi.getOperand(idx).getReg() & 7;
        return 0;
    };
    auto getImm = [&](unsigned idx) -> int64_t {
        if (idx < numOps && mi.getOperand(idx).isImm())
            return mi.getOperand(idx).getImm();
        return 0;
    };

    switch (opc) {
    // MOV64rr
    case X86::MOV64rr:
        out.push_back(0x48); out.push_back(0x89);
        emitModRM(3, getReg(0), getReg(1));
        break;
    // MOV64ri32
    case X86::MOV64ri32:
        out.push_back(0x48); out.push_back(0xC7);
        emitModRM(3, 0, getReg(1));
        emitImm32(getImm(0));
        break;
    // ADD64rr
    case X86::ADD64rr:
        out.push_back(0x48); out.push_back(0x01);
        emitModRM(3, getReg(0), getReg(1));
        break;
    // ADD64ri32
    case X86::ADD64ri32:
        out.push_back(0x48); out.push_back(0x81);
        emitModRM(3, 0, getReg(1));
        emitImm32(getImm(0));
        break;
    // SUB64rr
    case X86::SUB64rr:
        out.push_back(0x48); out.push_back(0x29);
        emitModRM(3, getReg(0), getReg(1));
        break;
    // SUB64ri32
    case X86::SUB64ri32:
        out.push_back(0x48); out.push_back(0x81);
        emitModRM(3, 5, getReg(1));
        emitImm32(getImm(0));
        break;
    // IMUL64rr
    case X86::IMUL64rr:
        out.push_back(0x48); out.push_back(0x0F); out.push_back(0xAF);
        emitModRM(3, getReg(1), getReg(0));
        break;
    // AND64rr / OR64rr / XOR64rr
    case X86::AND64rr: out.push_back(0x48); out.push_back(0x21); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::OR64rr:  out.push_back(0x48); out.push_back(0x09); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::XOR64rr: out.push_back(0x48); out.push_back(0x31); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::XOR32rr: out.push_back(0x31); emitModRM(3, getReg(0), getReg(1)); break;
    // AND64ri32 / OR64ri32 / XOR64ri32
    case X86::AND64ri32: out.push_back(0x48); out.push_back(0x81); emitModRM(3, 4, getReg(1)); emitImm32(getImm(0)); break;
    case X86::OR64ri32:  out.push_back(0x48); out.push_back(0x81); emitModRM(3, 1, getReg(1)); emitImm32(getImm(0)); break;
    case X86::XOR64ri32: out.push_back(0x48); out.push_back(0x81); emitModRM(3, 6, getReg(1)); emitImm32(getImm(0)); break;
    // CMP64rr / CMP64ri32
    case X86::CMP64rr:   out.push_back(0x48); out.push_back(0x39); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::CMP64ri32: out.push_back(0x48); out.push_back(0x81); emitModRM(3, 7, getReg(1)); emitImm32(getImm(0)); break;
    // RET
    case X86::RETQ: out.push_back(0xC3); break;
    // PUSH64r / POP64r
    case X86::PUSH64r: out.push_back(0x50 + getReg(0)); break;
    case X86::POP64r:  out.push_back(0x58 + getReg(0)); break;
    // JMP rel8 / JE rel8
    case X86::JMP_1: out.push_back(0xEB); out.push_back(0x00); break; // placeholder
    case X86::JE_1:  out.push_back(0x74); out.push_back(0x00); break;
    case X86::JNE_1: out.push_back(0x75); out.push_back(0x00); break;
    case X86::JL_1:  out.push_back(0x7C); out.push_back(0x00); break;
    case X86::JG_1:  out.push_back(0x7F); out.push_back(0x00); break;
    case X86::JLE_1: out.push_back(0x7E); out.push_back(0x00); break;
    case X86::JGE_1: out.push_back(0x7D); out.push_back(0x00); break;
    // SHL64ri / NOP
    case X86::SHL64ri: out.push_back(0x48); out.push_back(0xC1); emitModRM(3, 4, getReg(1)); emitImm32(getImm(0)); break;
    case X86::NOP: out.push_back(0x90); break;
    case X86::CQO: out.push_back(0x48); out.push_back(0x99); break;
    // MOVSX64rr32
    case X86::MOVSX64rr32: out.push_back(0x48); out.push_back(0x63); emitModRM(3, getReg(1), getReg(0)); break;
    // MOV64rm / MOV64mr
    case X86::MOV64rm: out.push_back(0x48); out.push_back(0x8B); emitModRM(3, getReg(0), getReg(1)); break;
    case X86::MOV64mr: out.push_back(0x48); out.push_back(0x89); emitModRM(3, getReg(1), getReg(0)); break;
    // IDIV64r
    case X86::IDIV64r: out.push_back(0x48); out.push_back(0xF7); emitModRM(3, 7, getReg(0)); break;
    default:
        out.push_back(0x90); // NOP placeholder for unknown
        break;
    }
}

void ObjectWriter::addRelocation(uint64_t offset, uint64_t symIdx, uint32_t type, int64_t addend) {
    relocs_.push_back({offset, symIdx, type, addend});
}

bool ObjectWriter::write(const std::string& path) {
    writeELF(path);
    return true;
}

void ObjectWriter::writeELF(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return;

    // Build section header string table
    std::string shstrtab;
    auto addShStr = [&](const std::string& s) -> uint32_t {
        uint32_t off = shstrtab.size();
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
        uint32_t off = strtab.size();
        strtab += s + '\0';
        return off;
    };
    addStr(""); // null string at index 0
    std::vector<uint32_t> symNameOffs = {0};
    for (size_t i = 1; i < symbols_.size(); i++)
        symNameOffs.push_back(addStr(symbols_[i].name));

    // Compute sizes and offsets
    uint64_t elfHdrOff = 0;
    uint64_t elfHdrSize = 64;
    uint64_t shdrOff = elfHdrSize;
    uint64_t shdrEntrySize = 64;
    uint16_t shdrCount = 7;
    uint64_t shdrTotalSize = shdrCount * shdrEntrySize;

    uint64_t textOff = shdrOff + shdrTotalSize;
    uint64_t dataOff = textOff + textBytes_.size();
    uint64_t symtabOff = dataOff + dataBytes_.size();
    uint64_t symtabSize = symbols_.size() * 24;
    uint64_t strtabOff = symtabOff + symtabSize;
    uint64_t relatextOff = strtabOff + strtab.size();
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

    // ---- .text section ----
    file.write(reinterpret_cast<const char*>(textBytes_.data()), textBytes_.size());

    // ---- .data section ----
    file.write(reinterpret_cast<const char*>(dataBytes_.data()), dataBytes_.size());

    // ---- .symtab section ----
    for (size_t i = 0; i < symbols_.size(); i++) {
        Elf64_Sym sym = {};
        sym.name = symNameOffs[i];
        sym.info = (symbols_[i].bind << 4) | (symbols_[i].type & 0xF);
        sym.other = STV_DEFAULT;
        sym.shndx = symbols_[i].shndx;
        sym.value = symbols_[i].value;
        sym.size = symbols_[i].size;
        file.write(reinterpret_cast<const char*>(&sym), sizeof(sym));
    }

    // ---- .strtab section ----
    file.write(strtab.data(), strtab.size());

    // ---- .rela.text section ----
    for (auto& r : relocs_) {
        Elf64_Rela rela = {};
        rela.offset = r.offset;
        rela.info = (r.symbolIndex << 32) | r.type;
        rela.addend = r.addend;
        file.write(reinterpret_cast<const char*>(&rela), sizeof(rela));
    }

    // ---- .shstrtab section ----
    file.write(shstrtab.data(), shstrtab.size());
}

} // namespace aurora
