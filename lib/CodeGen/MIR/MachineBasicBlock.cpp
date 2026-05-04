#include "Aurora/CodeGen/MachineBasicBlock.h"

namespace aurora {

MachineBasicBlock::MachineBasicBlock(const std::string& name)
    : name_(name), parent_(nullptr), first_(nullptr), last_(nullptr) {}

MachineBasicBlock::~MachineBasicBlock() {
    const MachineInstr* mi = first_;
    while (mi) {
        const MachineInstr* next = mi->getNext();
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

void MachineBasicBlock::remove(MachineInstr* mi) {
    if (mi->getPrev()) mi->getPrev()->setNext(mi->getNext());
    else first_ = mi->getNext();
    if (mi->getNext()) mi->getNext()->setPrev(mi->getPrev());
    else last_ = mi->getPrev();
    mi->setNext(nullptr);
    mi->setPrev(nullptr);
    mi->setParent(nullptr);
}

void MachineBasicBlock::addSuccessor(MachineBasicBlock* succ) {
    successors_.push_back(succ);
    succ->addPredecessor(this);
}

void MachineBasicBlock::addPredecessor(MachineBasicBlock* pred) {
    predecessors_.push_back(pred);
}

} // namespace aurora
