#include <gtest/gtest.h>
#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/CodeGen/PassManager.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Target/TargetMachine.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Constant.h"
#include <cstdio>
#include <fstream>
#include <memory>

using namespace aurora;

TEST(ObjectWriterTest, Construction) {
    ObjectWriter w;
    SUCCEED();
}

TEST(ObjectWriterTest, AddGlobal) {
    ObjectWriter w;
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "test", ConstantInt::getInt64(42));
    w.addGlobal(*gv);
    delete gv;
    SUCCEED();
}

TEST(ObjectWriterTest, AddExternSymbol) {
    ObjectWriter w;
    w.addExternSymbol("printf");
    w.addExternSymbol("printf"); // duplicate
    SUCCEED();
}

TEST(ObjectWriterTest, WriteEmptyFile) {
    ObjectWriter w;
    EXPECT_TRUE(w.write("/tmp/aurora_empty.o"));
    std::remove("/tmp/aurora_empty.o");
}

TEST(ObjectWriterTest, WriteReturnsFalseForInvalidPath) {
    ObjectWriter w;
    EXPECT_FALSE(w.write("/tmp/aurora_missing_dir/out.o"));
}

TEST(ObjectWriterTest, WriteWithGlobalOnly) {
    ObjectWriter w;
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "count", ConstantInt::getInt64(99));
    w.addGlobal(*gv);
    EXPECT_TRUE(w.write("/tmp/aurora_data.o"));
    delete gv;
    std::remove("/tmp/aurora_data.o");
}

TEST(ObjectWriterTest, AlignsSectionOffsets) {
    auto module = std::make_unique<Module>("align");
    auto* fn = module->createFunction(new FunctionType(Type::getInt64Ty(), {}), "f");
    AIRBuilder(fn->getEntryBlock()).createRet(0);

    auto tm = TargetMachine::createX86_64();
    MachineFunction mf(*fn, *tm);
    PassManager pm;
    CodeGenContext::addStandardPasses(pm, *tm);
    pm.run(mf);

    ObjectWriter writer;
    writer.addFunction(mf);
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "g", ConstantInt::getInt64(1));
    writer.addGlobal(*gv);
    EXPECT_TRUE(writer.write("/tmp/aurora_align.o"));

    std::ifstream file("/tmp/aurora_align.o", std::ios::binary);
    ASSERT_TRUE(file.good());

    struct Elf64_Ehdr {
        uint8_t ident[16];
        uint16_t type;
        uint16_t machine;
        uint32_t version;
        uint64_t entry;
        uint64_t phoff;
        uint64_t shoff;
        uint32_t flags;
        uint16_t ehsize;
        uint16_t phentsize;
        uint16_t phnum;
        uint16_t shentsize;
        uint16_t shnum;
        uint16_t shstrndx;
    };
    struct Elf64_Shdr {
        uint32_t name;
        uint32_t type;
        uint64_t flags;
        uint64_t addr;
        uint64_t offset;
        uint64_t size;
        uint32_t link;
        uint32_t info;
        uint64_t addralign;
        uint64_t entsize;
    };

    Elf64_Ehdr ehdr{};
    file.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));
    ASSERT_TRUE(file.good());
    file.seekg(static_cast<std::streamoff>(ehdr.shoff), std::ios::beg);

    std::vector<Elf64_Shdr> shdrs(ehdr.shnum);
    file.read(reinterpret_cast<char*>(shdrs.data()), static_cast<std::streamsize>(shdrs.size() * sizeof(Elf64_Shdr)));
    ASSERT_TRUE(file.good());

    EXPECT_EQ(shdrs[1].offset % shdrs[1].addralign, 0u);
    EXPECT_EQ(shdrs[2].offset % shdrs[2].addralign, 0u);
    EXPECT_EQ(shdrs[3].offset % shdrs[3].addralign, 0u);

    delete gv;
    std::remove("/tmp/aurora_align.o");
}

TEST(ObjectWriterTest, RelocationTypes) {
    EXPECT_EQ(R_X86_64_PC32, 2u);
    EXPECT_EQ(R_X86_64_PLT32, 4u);
    EXPECT_EQ(R_X86_64_32S, 11u);
}
