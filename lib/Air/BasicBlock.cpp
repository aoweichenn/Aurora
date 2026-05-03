#include "Aurora/Air/BasicBlock.h"
#include "Aurora/Air/Function.h"
#include <algorithm>

namespace aurora {

BasicBlock::BasicBlock(const std::string& name)
    : name_(name), parent_(nullptr), first_(nullptr), last_(nullptr) {}

BasicBlock::~BasicBlock() {
    AIRInstruction* inst = first_;
    while (inst) {
        AIRInstruction* next = inst->getNext();
        delete inst;
        inst = next;
    }
}

AIRInstruction* BasicBlock::getTerminator() const {
    AIRInstruction* t = last_;
    while (t) {
        switch (t->getOpcode()) {
        case AIROpcode::Ret:
        case AIROpcode::Br:
        case AIROpcode::CondBr:
        case AIROpcode::Unreachable:
            return t;
        default:
            break;
        }
        t = t->getPrev();
    }
    return nullptr;
}

void BasicBlock::pushBack(AIRInstruction* inst) {
    inst->setParent(this);
    if (!first_) {
        first_ = last_ = inst;
        inst->setPrev(nullptr);
        inst->setNext(nullptr);
    } else {
        last_->setNext(inst);
        inst->setPrev(last_);
        inst->setNext(nullptr);
        last_ = inst;
    }
}

void BasicBlock::pushFront(AIRInstruction* inst) {
    inst->setParent(this);
    if (!first_) {
        first_ = last_ = inst;
        inst->setPrev(nullptr);
        inst->setNext(nullptr);
    } else {
        inst->setNext(first_);
        inst->setPrev(nullptr);
        first_->setPrev(inst);
        first_ = inst;
    }
}

void BasicBlock::insertBefore(AIRInstruction* pos, AIRInstruction* inst) {
    inst->setParent(this);
    inst->setPrev(pos->getPrev());
    inst->setNext(pos);
    if (pos->getPrev()) pos->getPrev()->setNext(inst);
    else first_ = inst;
    pos->setPrev(inst);
}

void BasicBlock::insertAfter(AIRInstruction* pos, AIRInstruction* inst) {
    inst->setParent(this);
    inst->setNext(pos->getNext());
    inst->setPrev(pos);
    if (pos->getNext()) pos->getNext()->setPrev(inst);
    else last_ = inst;
    pos->setNext(inst);
}

void BasicBlock::erase(AIRInstruction* inst) {
    if (inst->getPrev()) inst->getPrev()->setNext(inst->getNext());
    else first_ = inst->getNext();
    if (inst->getNext()) inst->getNext()->setPrev(inst->getPrev());
    else last_ = inst->getPrev();
    delete inst;
}

void BasicBlock::addSuccessor(BasicBlock* succ) {
    successors_.push_back(succ);
    succ->addPredecessor(this);
}

void BasicBlock::addPredecessor(BasicBlock* pred) {
    predecessors_.push_back(pred);
}

void BasicBlock::removePredecessor(BasicBlock* pred) {
    auto it = std::find(predecessors_.begin(), predecessors_.end(), pred);
    if (it != predecessors_.end()) {
        predecessors_.erase(it);
        auto sit = std::find(pred->successors_.begin(), pred->successors_.end(), this);
        if (sit != pred->successors_.end())
            pred->successors_.erase(sit);
    }
}

} // namespace aurora
