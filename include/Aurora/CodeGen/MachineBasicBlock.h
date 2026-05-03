#pragma once

#include <string>
#include "Aurora/ADT/BitVector.h"
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

class MachineFunction;

class MachineBasicBlock {
public:
    explicit MachineBasicBlock(const std::string& name = "");
    MachineBasicBlock(const MachineBasicBlock&) = delete;
    MachineBasicBlock& operator=(const MachineBasicBlock&) = delete;
    ~MachineBasicBlock();

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] MachineFunction* getParent() const noexcept { return parent_; }
    void setParent(MachineFunction* mf) noexcept { parent_ = mf; }

    [[nodiscard]] bool empty() const noexcept { return first_ == nullptr; }
    [[nodiscard]] MachineInstr* getFirst() const noexcept { return first_; }
    [[nodiscard]] MachineInstr* getLast() const noexcept { return last_; }

    void pushBack(MachineInstr* mi);
    void insertAfter(MachineInstr* pos, MachineInstr* mi);
    void insertBefore(MachineInstr* pos, MachineInstr* mi);

    void addSuccessor(MachineBasicBlock* succ);
    [[nodiscard]] const SmallVector<MachineBasicBlock*, 4>& successors() const noexcept { return successors_; }
    [[nodiscard]] const SmallVector<MachineBasicBlock*, 4>& predecessors() const noexcept { return predecessors_; }
    void addPredecessor(MachineBasicBlock* pred);

    [[nodiscard]] BitVector& getLiveIns() noexcept { return liveIns_; }
    [[nodiscard]] BitVector& getLiveOuts() noexcept { return liveOuts_; }

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

