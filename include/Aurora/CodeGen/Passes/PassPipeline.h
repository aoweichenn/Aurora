#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "Aurora/CodeGen/PassManager.h"

namespace aurora {

class PassPipeline {
public:
    using PassFactory = std::function<std::unique_ptr<CodeGenPass>()>;

    PassPipeline& add(PassFactory factory);
    void build(PassManager& pm) const;

    [[nodiscard]] unsigned size() const noexcept;
    [[nodiscard]] static PassPipeline standardCodeGenPipeline();

private:
    std::vector<PassFactory> factories_;
};

} // namespace aurora
