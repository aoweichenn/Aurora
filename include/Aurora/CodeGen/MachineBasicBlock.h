#ifndef AURORA_CODEGEN_MACHINEBASICBLOCK_H
#define AURORA_CODEGEN_MACHINEBASICBLOCK_H

#include "Aurora/CodeGen/MachineInstr.h"
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/ADT/BitVector.h"
#include <string>

namespace aurora {

class MachineFunction;

class MachineBasicBlock {
public:
    explicit MachineBasicBlock(const std::string& name = "");
    MachineBasicBlock(const MachineBasicBlock&) = delete;
    MachineBasicBlock& operator=(const MachineBasicBlock&) = delete;
    ~MachineBasicBlock();

    const std::string& getName() const noexcept { return name_; }
    MachineFunction* getParent() const noexcept { return parent_; }
    void setParent(MachineFunction* mf) noexcept { parent_ = mf; }

    bool empty() const noexcept { return first_ == nullptr; }
    MachineInstr* getFirst() const noexcept { return first_; }
    MachineInstr* getLast() const noexcept { return last_; }

    void pushBack(MachineInstr* mi);
    void insertAfter(MachineInstr* pos, MachineInstr* mi);
    void insertBefore(MachineInstr* pos, MachineInstr* mi);

    void addSuccessor(MachineBasicBlock* succ);
    const SmallVector<MachineBasicBlock*, 4>& successors() const noexcept { return successors_; }
    const SmallVector<MachineBasicBlock*, 4>& predecessors() const noexcept { return predecessors_; }
    void addPredecessor(MachineBasicBlock* pred);

    BitVector& getLiveIns() noexcept { return liveIns_; }
    BitVector& getLiveOuts() noexcept { return liveOuts_; }

private:
    std::string name_;
    MachineFunction* parent_;
    MachineInstr* first_;
    MachineInstr* last_;
    SmallVector<MachineBasicBlock*, 4> successors_;
    SmallVector<MachineBasicBlock*, 4> predecessors_;
    BitVector liveIns_;
    BitVector liveOuts_;
};

} // namespace aurora

#endif // AURORA_CODEGEN_MACHINEBASICBLOCK_H
