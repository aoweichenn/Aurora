#include "Aurora/Target/TargetLowering.h"
#include "Aurora/Air/Instruction.h"
#include <cstring>

namespace aurora {

void TargetLowering::setOperationAction(AIROpcode op, unsigned vtSize, LegalizeAction action) {
    // Subclass must implement initActions() to fill its own tables
}

void TargetLowering::setTypeLegal(unsigned bits, bool legal) {
    // Subclass must implement initActions() to fill its own tables
}

} // namespace aurora
