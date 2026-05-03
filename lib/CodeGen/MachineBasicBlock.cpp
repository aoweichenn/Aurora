#include "Aurora/CodeGen/MachineBasicBlock.h"

namespace aurora {

MachineBasicBlock::MachineBasicBlock(const std::string& name)
    : name_(name), parent_(nullptr), first_(nullptr), last_(nullptr) {}

MachineBasicBlock::~MachineBasicBlock() {
    MachineInstr* mi = first_;
    while (mi) {
        MachineInstr* next = mi->getNext();
        delete mi;
        mi = next;
    }
}

void MachineBasicBlock::pushBack(MachineInstr* mi) {
    mi->setParent(this);
    if (!first_) {
        first_ = last_ = mi;
        mi->setPrev(nullptr);
        mi->setNext(nullptr);
    } else {
        last_->setNext(mi);
        mi->setPrev(last_);
        mi->setNext(nullptr);
        last_ = mi;
    }
}

void MachineBasicBlock::insertAfter(MachineInstr* pos, MachineInstr* mi) {
    mi->setParent(this);
    mi->setNext(pos->getNext());
    mi->setPrev(pos);
    if (pos->getNext()) pos->getNext()->setPrev(mi);
    else last_ = mi;
    pos->setNext(mi);
}

void MachineBasicBlock::insertBefore(MachineInstr* pos, MachineInstr* mi) {
    mi->setParent(this);
    mi->setPrev(pos->getPrev());
    mi->setNext(pos);
    if (pos->getPrev()) pos->getPrev()->setNext(mi);
    else first_ = mi;
    pos->setPrev(mi);
}

void MachineBasicBlock::addSuccessor(MachineBasicBlock* succ) {
    successors_.push_back(succ);
    succ->addPredecessor(this);
}

void MachineBasicBlock::addPredecessor(MachineBasicBlock* pred) {
    predecessors_.push_back(pred);
}

} // namespace aurora
