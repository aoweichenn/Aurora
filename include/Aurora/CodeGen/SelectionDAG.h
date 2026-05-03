#ifndef AURORA_CODEGEN_SELECTIONDAG_H
#define AURORA_CODEGEN_SELECTIONDAG_H

#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Instruction.h"
#include <vector>

namespace aurora {

class BasicBlock;
class MachineBasicBlock;
class MachineInstr;
class Type;

class SDValue;

class SDNode {
public:
    SDNode(AIROpcode op, Type* ty);
    ~SDNode();

    AIROpcode getOpcode() const noexcept { return opcode_; }
    Type* getType() const noexcept { return type_; }
    unsigned getNodeId() const noexcept { return nodeId_; }
    void setNodeId(const unsigned id) noexcept { nodeId_ = id; }

    unsigned getNumOperands() const noexcept { return static_cast<unsigned>(operands_.size()); }
    SDValue getOperand(unsigned i) const;
    void addOperand(SDValue val);

    const SmallVector<SDNode*, 4>& getUsers() const noexcept { return users_; }
    void addUser(SDNode* user);

    bool isSelected() const noexcept { return selected_; }
    void setSelected(const bool s) noexcept { selected_ = s; }
    uint16_t getMachineOpcode() const noexcept { return machOpcode_; }
    void setMachineOpcode(const uint16_t op) noexcept { machOpcode_ = op; }

    unsigned getVReg() const noexcept { return vreg_; }
    void setVReg(const unsigned v) noexcept { vreg_ = v; }

    void createMachineInstr(uint16_t opcode);

private:
    AIROpcode opcode_;
    Type* type_;
    unsigned nodeId_;
    unsigned vreg_;
    SmallVector<SDNode*, 4> operands_;
    SmallVector<SDNode*, 4> users_;
    bool selected_;
    uint16_t machOpcode_;
};

class SDValue {
public:
    SDValue() : node_(nullptr), resNo_(0) {}
    SDValue(SDNode* node, const unsigned resNo = 0) : node_(node), resNo_(resNo) {}

    SDNode* getNode() const noexcept { return node_; }
    unsigned getResNo() const noexcept { return resNo_; }
    bool isValid() const noexcept { return node_ != nullptr; }

private:
    SDNode* node_;
    unsigned resNo_;
};

class SelectionDAG {
public:
    SelectionDAG();
    ~SelectionDAG();

    SDValue createNode(AIROpcode op, Type* ty, const SmallVector<SDValue, 4>& ops);
    SDValue createConstant(int64_t val, Type* ty);
    SDValue createRegister(unsigned vreg, Type* ty);

    void buildFromBasicBlock(BasicBlock* airBB, MachineBasicBlock* mbb);
    const std::vector<SDNode*>& getAllNodes() const noexcept { return allNodes_; }
    const SmallVector<SDValue, 4>& getRoots() const noexcept { return roots_; }

    void dagCombine();
    void legalize();
    void select(MachineBasicBlock* mbb);
    void schedule(MachineBasicBlock* mbb);

private:
    unsigned nextNodeId_;
    std::vector<SDNode*> allNodes_;
    SmallVector<SDValue, 4> roots_;

    SDNode* createSDNode(AIROpcode op, Type* ty);
};

} // namespace aurora

#endif // AURORA_CODEGEN_SELECTIONDAG_H
