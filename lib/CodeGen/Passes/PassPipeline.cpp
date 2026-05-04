#include "Aurora/CodeGen/Passes/PassPipeline.h"
#include "Aurora/CodeGen/Passes/PassFactories.h"

namespace aurora {

PassPipeline& PassPipeline::add(PassFactory factory) {
    factories_.push_back(std::move(factory));
    return *this;
}

void PassPipeline::build(PassManager& pm) const {
    for (const auto& factory : factories_)
        pm.addPass(factory());
}

unsigned PassPipeline::size() const noexcept {
    return static_cast<unsigned>(factories_.size());
}

PassPipeline PassPipeline::standardCodeGenPipeline() {
    PassPipeline pipeline;
    pipeline.add(createAIRToMachineIRPass)
            .add(createInstructionSelectionPass)
            .add(createRegisterAllocationPass)
            .add(createPrologueEpilogueInsertionPass)
            .add(createBranchFoldingPass);
    return pipeline;
}

} // namespace aurora
