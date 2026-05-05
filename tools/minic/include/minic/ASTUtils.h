#pragma once

#include "minic/AST.h"
#include <cstdint>

namespace minic {

[[nodiscard]] bool isAssignableExpr(const Expr& expr) noexcept;
[[nodiscard]] uint64_t sizeOfType(CType type) noexcept;

} // namespace minic
