#pragma once

#include <memory>
#include <vector>

namespace aurora {

class MachineFunction;
class Module;
class TargetMachine;

class CodeGenPass {
public:
    virtual ~CodeGenPass() = default;
    virtual void run(MachineFunction& mf) = 0;
    [[nodiscard]] virtual const char* getName() const = 0;
};

class PassManager {
public:
    void addPass(std::unique_ptr<CodeGenPass> pass);
    void run(MachineFunction& mf);

private:
    std::vector<std::unique_ptr<CodeGenPass>> passes_;
};

class CodeGenContext {
public:
    CodeGenContext(TargetMachine& tm, Module& module);
    void run();

    [[nodiscard]] TargetMachine& getTarget() noexcept { return target_; }
    [[nodiscard]] Module& getModule() noexcept { return module_; }

    static void addStandardPasses(PassManager& pm, TargetMachine& tm);

private:
    TargetMachine& target_;
    Module& module_;
};

} // namespace aurora

