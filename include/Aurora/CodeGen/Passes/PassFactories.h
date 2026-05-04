#pragma once

#include <memory>
#include "Aurora/CodeGen/PassManager.h"

namespace aurora {

[[nodiscard]] std::unique_ptr<CodeGenPass> createAIRToMachineIRPass();
[[nodiscard]] std::unique_ptr<CodeGenPass> createInstructionSelectionPass();
[[nodiscard]] std::unique_ptr<CodeGenPass> createRegisterAllocationPass();
[[nodiscard]] std::unique_ptr<CodeGenPass> createPrologueEpilogueInsertionPass();
[[nodiscard]] std::unique_ptr<CodeGenPass> createBranchFoldingPass();

} // namespace aurora
