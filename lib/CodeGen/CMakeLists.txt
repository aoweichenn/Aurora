add_library(AuroraCodeGen STATIC
    MIR/MachineInstr.cpp
    MIR/MachineBasicBlock.cpp
    MIR/MachineFunction.cpp
    MIR/SelectionDAG.cpp
    InstructionSelector.cpp
    RA/LiveInterval.cpp
    RA/RegisterAllocator.cpp
    Frame/PrologueEpilogueInserter.cpp
    ISel/ISelContext.cpp
    ISel/LoweringStrategy.cpp
    ISel/X86LoweringStrategies.cpp
    ISel/X86OpcodeMapper.cpp
    Passes/PassPipeline.cpp
    Passes/PassManager.cpp
)
target_link_libraries(AuroraCodeGen PUBLIC AuroraTarget AuroraX86)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraCodeGen_shared SHARED
        MIR/MachineInstr.cpp
        MIR/MachineBasicBlock.cpp
        MIR/MachineFunction.cpp
        MIR/SelectionDAG.cpp
        InstructionSelector.cpp
        RA/LiveInterval.cpp
        RA/RegisterAllocator.cpp
        Frame/PrologueEpilogueInserter.cpp
        ISel/ISelContext.cpp
        ISel/LoweringStrategy.cpp
        ISel/X86LoweringStrategies.cpp
        ISel/X86OpcodeMapper.cpp
        Passes/PassPipeline.cpp
        Passes/PassManager.cpp
    )
    target_link_libraries(AuroraCodeGen_shared PUBLIC AuroraTarget AuroraX86)
    set_target_properties(AuroraCodeGen_shared PROPERTIES OUTPUT_NAME AuroraCodeGen)
endif()

install(TARGETS AuroraCodeGen DESTINATION lib)
