#include "Aurora/Target/TargetLowering.h"

namespace aurora {

void TargetLowering::setOperationAction(AIROpcode /*op*/, unsigned /*vtSize*/, LegalizeAction /*action*/) const
{
    // Subclass must implement initActions() to fill its own tables
}

void TargetLowering::setTypeLegal(unsigned /*bits*/, bool /*legal*/) const
{
    // Subclass must implement initActions() to fill its own tables
}

} // namespace aurora
