#pragma once

namespace aurora {

class MachineFunction;

class PrologueEpilogueInserter {
public:
    void run(MachineFunction& mf);
};

} // namespace aurora

