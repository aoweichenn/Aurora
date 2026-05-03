#include "Aurora/CodeGen/SelectionDAG.h"
#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include "Aurora/CodeGen/MachineBasicBlock.h"
#include "Aurora/CodeGen/MachineInstr.h"

namespace aurora {

SDNode::SDNode(AIROpcode op, Type* ty)
    : opcode_(op), type_(ty), nodeId_(0), vreg_(~0U),
      selected_(false), machOpcode_(0) {}

SDNode::~SDNode() = default;

SDValue SDNode::getOperand(unsigned i) const {
    return SDValue(operands_[i], 0);
}

void SDNode::addOperand(SDValue val) {
    operands_.push_back(val.getNode());
    if (val.getNode())
        const_cast<SDNode*>(val.getNode())->addUser(this);
}

void SDNode::addUser(SDNode* user) {
    users_.push_back(user);
}

void SDNode::createMachineInstr(uint16_t opcode) {
    machOpcode_ = opcode;
    selected_ = true;
}

SelectionDAG::SelectionDAG() : nextNodeId_(0) {}
SelectionDAG::~SelectionDAG() {
    for (auto* n : allNodes_) delete n;
}

SDNode* SelectionDAG::createSDNode(AIROpcode op, Type* ty) {
    auto* node = new SDNode(op, ty);
    node->setNodeId(nextNodeId_++);
    allNodes_.push_back(node);
    return node;
}

SDValue SelectionDAG::createNode(AIROpcode op, Type* ty, const SmallVector<SDValue, 4>& ops) {
    auto* node = createSDNode(op, ty);
    for (const auto& o : ops) {
        node->addOperand(o);
    }
    return SDValue(node);
}

SDValue SelectionDAG::createConstant(int64_t val, Type* ty) {
    auto* node = createSDNode(AIROpcode::Add, ty); // Placeholder - we use a generic node for constants
    node->setVReg(val); // Store constant value in vreg field temporarily
    return SDValue(node);
}

SDValue SelectionDAG::createRegister(unsigned vreg, Type* ty) {
    auto* node = createSDNode(AIROpcode::Add, ty);
    node->setVReg(vreg);
    return SDValue(node);
}

void SelectionDAG::buildFromBasicBlock(BasicBlock* airBB, MachineBasicBlock* mbb) {
    // Walk AIR instructions and build DAG nodes
    // For each AIR instruction, create corresponding SDNode
    AIRInstruction* inst = airBB->getFirst();
    while (inst) {
        AIROpcode op = inst->getOpcode();
        Type* ty = inst->getType();

        SmallVector<SDValue, 4> ops;
        for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
            unsigned vreg = inst->getOperand(i);
            if (vreg != ~0U) {
                ops.push_back(createRegister(vreg, ty));
            }
        }

        if (inst->hasResult()) {
            auto sv = createNode(op, ty, ops);
            roots_.push_back(sv);
        } else {
            createNode(op, ty, ops);
            roots_.push_back(SDValue(allNodes_.back()));
        }

        inst = inst->getNext();
    }
}

void SelectionDAG::dagCombine() {
    // Merge redundant operations
    // Example: shift + add → LEA on x86
}

void SelectionDAG::legalize() {
    // Legalize types and operations per target
}

void SelectionDAG::select(MachineBasicBlock* mbb) {
    // Pattern-match each SDNode to MachineInstr
    for (auto* node : allNodes_) {
        if (node->isSelected()) continue;
        // Pattern matching happens here
    }
}

void SelectionDAG::schedule(MachineBasicBlock* mbb) {
    // Topological sort of DAG → linear instruction sequence
    for (const auto& root : roots_) {
        if (root.getNode()) {
            // Emit machine instruction
        }
    }
}

} // namespace aurora
