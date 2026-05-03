#include <gtest/gtest.h>
#include "Aurora/CodeGen/InstructionSelector.h"
#include "Aurora/Target/TargetMachine.h"

using namespace aurora;

TEST(InstructionSelectorTest, Construction) {
    const auto tm = TargetMachine::createX86_64();
    InstructionSelector isel(*tm);
    SUCCEED();
}
