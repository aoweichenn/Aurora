#include <gtest/gtest.h>
#include "Aurora/MC/ObjectWriter.h"
#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineFunction.h"
#include "Aurora/Target/X86/X86InstrInfo.h"
#include "Aurora/Target/X86/X86RegisterInfo.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Type.h"
#include "Aurora/Air/Constant.h"
#include <cstdio>

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

TEST(ObjectWriterTest, WriteWithGlobalOnly) {
    ObjectWriter w;
    auto* gv = new GlobalVariable(Type::getInt64Ty(), "count", ConstantInt::getInt64(99));
    w.addGlobal(*gv);
    EXPECT_TRUE(w.write("/tmp/aurora_data.o"));
    delete gv;
    std::remove("/tmp/aurora_data.o");
}

TEST(ObjectWriterTest, RelocationTypes) {
    EXPECT_EQ(R_X86_64_PC32, 2u);
    EXPECT_EQ(R_X86_64_PLT32, 4u);
    EXPECT_EQ(R_X86_64_32S, 11u);
}
