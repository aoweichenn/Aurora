#ifndef AURORA_CODEGEN_PROLOGUEEPILOGUEINSERTER_H
#define AURORA_CODEGEN_PROLOGUEEPILOGUEINSERTER_H

namespace aurora {

class MachineFunction;

class PrologueEpilogueInserter {
public:
    void run(MachineFunction& mf);
};

} // namespace aurora

#endif // AURORA_CODEGEN_PROLOGUEEPILOGUEINSERTER_H
