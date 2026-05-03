#pragma once

#include <cstdint>

namespace aurora {

class Type;
enum class AIROpcode : uint16_t;

enum class LegalizeAction : uint8_t {
    Legal,
    Promote,
    Expand,
    LibCall,
    Custom
};

class TargetLowering {
public:
    virtual ~TargetLowering() = default;

    [[nodiscard]] virtual LegalizeAction getOperationAction(AIROpcode op, unsigned vtSize) const = 0;
    [[nodiscard]] virtual bool isTypeLegal(unsigned typeSize) const = 0;
    [[nodiscard]] virtual unsigned getRegisterSizeForType(Type* ty) const = 0;

protected:
    void setOperationAction(AIROpcode op, unsigned vtSize, LegalizeAction action);
    void setTypeLegal(unsigned bits, bool legal);

private:
    // 内部使用，由子类通过 setOperationAction 填充
    virtual void initActions() = 0;
};

} // namespace aurora

