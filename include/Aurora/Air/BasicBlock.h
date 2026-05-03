#ifndef AURORA_AIR_BASICBLOCK_H
#define AURORA_AIR_BASICBLOCK_H

#include "Aurora/Air/Instruction.h"
#include "Aurora/ADT/SmallVector.h"
#include <string>

namespace aurora {

class Function;

class BasicBlock {
public:
    explicit BasicBlock(const std::string& name = "");
    ~BasicBlock();

    const std::string& getName() const noexcept { return name_; }
    Function* getParent() const noexcept { return parent_; }
    void setParent(Function* f) noexcept { parent_ = f; }

    // Instruction list
    AIRInstruction* getFirst() const noexcept { return first_; }
    AIRInstruction* getLast()  const noexcept { return last_; }
    AIRInstruction* getTerminator() const;

    bool empty() const noexcept { return first_ == nullptr; }

    void pushBack(AIRInstruction* inst);
    void pushFront(AIRInstruction* inst);
    void insertBefore(AIRInstruction* pos, AIRInstruction* inst);
    void insertAfter(AIRInstruction* pos, AIRInstruction* inst);
    void erase(AIRInstruction* inst);

    // CFG
    void addSuccessor(BasicBlock* succ);
    const SmallVector<BasicBlock*, 4>& getSuccessors() const noexcept { return successors_; }
    const SmallVector<BasicBlock*, 4>& getPredecessors() const noexcept { return predecessors_; }
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

#endif // AURORA_AIR_BASICBLOCK_H
