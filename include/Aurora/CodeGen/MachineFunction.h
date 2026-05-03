#pragma once

#include <memory>
#include <vector>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Type.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"

namespace aurora {

class Function;
class TargetMachine;
class LiveInterval;

struct StackObject {
    unsigned size;
    unsigned alignment;
    int frameIndex;
};

class MachineFunction {
public:
    MachineFunction(Function& airFunc, const TargetMachine& TM);
    ~MachineFunction();

    [[nodiscard]] Function& getAIRFunction() const noexcept { return airFunc_; }
    [[nodiscard]] const TargetMachine& getTarget() const noexcept { return target_; }

    [[nodiscard]] MachineBasicBlock* createBasicBlock(const std::string& name = "");
    [[nodiscard]] SmallVector<std::unique_ptr<MachineBasicBlock>, 8>& getBlocks() noexcept { return blocks_; }
    [[nodiscard]] const SmallVector<std::unique_ptr<MachineBasicBlock>, 8>& getBlocks() const noexcept { return blocks_; }

    [[nodiscard]] unsigned createVirtualRegister(Type* ty);
    [[nodiscard]] Type* getVRegType(unsigned vreg) const noexcept;

    [[nodiscard]] int createStackSlot(unsigned size, unsigned alignment);
    [[nodiscard]] const std::vector<StackObject>& getStackObjects() const noexcept { return stackObjects_; }

    [[nodiscard]] unsigned getNumVRegs() const noexcept { return nextVReg_; }

private:
    Function& airFunc_;
    const TargetMachine& target_;
    SmallVector<std::unique_ptr<MachineBasicBlock>, 8> blocks_;
    unsigned nextVReg_;
    std::vector<Type*> vregTypes_;
    std::vector<StackObject> stackObjects_;
    int nextFrameIndex_;
};

} // namespace aurora

