#pragma once

#include <cstdint>
#include "Aurora/ADT/SmallVector.h"

namespace aurora {

class Register;
class MachineBasicBlock;

enum class MachineOperandKind : uint8_t {
    MO_Register,
    MO_VirtualReg,
    MO_Immediate,
    MO_MBB,
    MO_GlobalSym,
    MO_FrameIndex,
    MO_None
};

class MachineOperand {
public:
    MachineOperand() : kind_(MachineOperandKind::MO_None), regId(0) {}
    [[nodiscard]] static MachineOperand createReg(unsigned regId);
    [[nodiscard]] static MachineOperand createVReg(unsigned vreg);
    [[nodiscard]] static MachineOperand createImm(int64_t val);
    [[nodiscard]] static MachineOperand createMBB(MachineBasicBlock* mbb);
    [[nodiscard]] static MachineOperand createFrameIndex(int idx);
    [[nodiscard]] static MachineOperand createGlobalSym(const char* sym);

    [[nodiscard]] MachineOperandKind getKind() const noexcept { return kind_; }
    [[nodiscard]] unsigned getReg() const noexcept;
    [[nodiscard]] unsigned getVirtualReg() const noexcept;
    [[nodiscard]] int64_t getImm() const noexcept;
    [[nodiscard]] MachineBasicBlock* getMBB() const noexcept;
    [[nodiscard]] const char* getGlobalSym() const noexcept;
    [[nodiscard]] int getFrameIndex() const noexcept;

    [[nodiscard]] bool isReg() const noexcept;
    [[nodiscard]] bool isVReg() const noexcept;
    [[nodiscard]] bool isImm() const noexcept;

private:
    MachineOperandKind kind_;
    union {
        unsigned regId;
        unsigned vregId;
        int64_t immVal;
        MachineBasicBlock* mbb;
        const char* globalSym;
        int frameIdx;
    };
};

class MachineInstr {
public:
    explicit MachineInstr(uint16_t opcode);
    ~MachineInstr();

    [[nodiscard]] uint16_t getOpcode() const noexcept { return opcode_; }
    [[nodiscard]] unsigned getNumOperands() const noexcept { return static_cast<unsigned>(operands_.size()); }
    [[nodiscard]] MachineOperand& getOperand(unsigned i);
    [[nodiscard]] const MachineOperand& getOperand(unsigned i) const;

    void addOperand(MachineOperand mo);
    void setOperand(unsigned i, MachineOperand mo);

    [[nodiscard]] MachineBasicBlock* getParent() const noexcept { return parent_; }
    void setParent(MachineBasicBlock* bb) noexcept { parent_ = bb; }

    [[nodiscard]] MachineInstr* getNext() const noexcept { return next_; }
    [[nodiscard]] MachineInstr* getPrev() const noexcept { return prev_; }
    void setNext(MachineInstr* n) noexcept { next_ = n; }
    void setPrev(MachineInstr* p) noexcept { prev_ = p; }

    [[nodiscard]] bool isTerminator() const noexcept;
    [[nodiscard]] bool isBranch() const noexcept;
    [[nodiscard]] bool isReturn() const noexcept;
    [[nodiscard]] bool isCall() const noexcept;
    [[nodiscard]] bool isMove() const noexcept;

private:
    uint16_t opcode_;
    SmallVector<MachineOperand, 4> operands_;
    MachineBasicBlock* parent_;
    MachineInstr* next_;
    MachineInstr* prev_;
};

} // namespace aurora

