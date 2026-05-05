#pragma once

#include "minic/ast/AST.h"
#include <cstdint>
#include <string>

namespace minic {

[[nodiscard]] bool isAssignableExpr(const Expr& expr) noexcept;
[[nodiscard]] uint64_t sizeOfType(CType type) noexcept;
[[nodiscard]] uint64_t alignOfType(CType type) noexcept;
[[nodiscard]] const CField* findStructField(const CType& type, const std::string& name) noexcept;

} // namespace minic
