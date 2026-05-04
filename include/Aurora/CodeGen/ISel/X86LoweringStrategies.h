#pragma once

#include <memory>
#include "Aurora/CodeGen/ISel/LoweringStrategy.h"

namespace aurora {

[[nodiscard]] std::unique_ptr<LoweringStrategy> createX86ConstantLoweringStrategy();

} // namespace aurora
