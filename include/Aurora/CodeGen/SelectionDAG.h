#pragma once

#include <vector>
#include "Aurora/ADT/SmallVector.h"
#include "Aurora/Air/Instruction.h"

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

    [[nodiscard]] AIROpcode getOpcode() const noexcept { return opcode_; }
    [[nodiscard]] Type* getType() const noexcept { return type_; }
    [[nodiscard]] unsigned getNodeId() const noexcept { return nodeId_; }
    void setNodeId(const unsigned id) noexcept { nodeId_ = id; }

    [[nodiscard]] unsigned getNumOperands() const noexcept { return static_cast<unsigned>(operands_.size()); }
    [[nodiscard]] SDValue getOperand(unsigned i) const;
    void addOperand(SDValue val);

    [[nodiscard]] const SmallVector<SDNode*, 4>& getUsers() const noexcept { return users_; }
    void addUser(SDNode* user);

    [[nodiscard]] bool isSelected() const noexcept { return selected_; }
    void setSelected(const bool s) noexcept { selected_ = s; }
    [[nodiscard]] uint16_t getMachineOpcode() const noexcept { return machOpcode_; }
    void setMachineOpcode(const uint16_t op) noexcept { machOpcode_ = op; }

    [[nodiscard]] unsigned getVReg() const noexcept { return vreg_; }
    void setVReg(const unsigned v) noexcept { vreg_ = v; }

    [[nodiscard]] bool hasConstantValue() const noexcept { return hasConstantValue_; }
    [[nodiscard]] int64_t getConstantValue() const noexcept { return constantValue_; }
    void setConstantValue(int64_t value) noexcept;

    void createMachineInstr(uint16_t opcode);

private:
    AIROpcode opcode_;
    Type* type_;
    unsigned nodeId_;
    unsigned vreg_;
    SmallVector<SDNode*, 4> operands_;
    SmallVector<SDNode*, 4> users_;
    int64_t constantValue_;
    bool hasConstantValue_;
    bool selected_;
    uint16_t machOpcode_;
};

class SDValue {
public:
    SDValue() : node_(nullptr), resNo_(0) {}
    SDValue(SDNode* node, const unsigned resNo = 0) : node_(node), resNo_(resNo) {}

    [[nodiscard]] SDNode* getNode() const noexcept { return node_; }
    [[nodiscard]] unsigned getResNo() const noexcept { return resNo_; }
    [[nodiscard]] bool isValid() const noexcept { return node_ != nullptr; }

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

    void dagCombine() const;
    void legalize() const;
    void select(MachineBasicBlock* mbb) const;
    void schedule(MachineBasicBlock* mbb);

private:
    unsigned nextNodeId_;
    std::vector<SDNode*> allNodes_;
    SmallVector<SDValue, 4> roots_;

    [[nodiscard]] SDNode* createSDNode(AIROpcode op, Type* ty);
};

} // namespace aurora

