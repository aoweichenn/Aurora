#pragma once

#include <string>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Instruction.h"

namespace aurora {

class Function;

class BasicBlock {
public:
    explicit BasicBlock(const std::string& name = "");
    BasicBlock(const BasicBlock&) = delete;
    BasicBlock& operator=(const BasicBlock&) = delete;
    ~BasicBlock();

    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] Function* getParent() const noexcept { return parent_; }
    void setParent(Function* f) noexcept { parent_ = f; }

    // Instruction list
    [[nodiscard]] AIRInstruction* getFirst() const noexcept { return first_; }
    [[nodiscard]] AIRInstruction* getLast()  const noexcept { return last_; }
    [[nodiscard]] AIRInstruction* getTerminator() const;

    [[nodiscard]] bool empty() const noexcept { return first_ == nullptr; }

    void pushBack(AIRInstruction* inst);
    void pushFront(AIRInstruction* inst);
    void insertBefore(AIRInstruction* pos, AIRInstruction* inst);
    void insertAfter(AIRInstruction* pos, AIRInstruction* inst);
    void erase(AIRInstruction* inst);

    // CFG
    void addSuccessor(BasicBlock* succ);
    [[nodiscard]] const SmallVector<BasicBlock*, 4>& getSuccessors() const noexcept { return successors_; }
    [[nodiscard]] const SmallVector<BasicBlock*, 4>& getPredecessors() const noexcept { return predecessors_; }
    void addPredecessor(BasicBlock* pred);
    void removePredecessor(BasicBlock* pred);

private:
    std::string name_;
    Function* parent_;
    AIRInstruction* first_;
    AIRInstruction* last_;
    SmallVector<BasicBlock*, 4> successors_;
    SmallVector<BasicBlock*, 4> predecessors_;
};

} // namespace aurora

