add_library(AuroraCodeGen STATIC
    MachineInstr.cpp
    MachineBasicBlock.cpp
    MachineFunction.cpp
    SelectionDAG.cpp
    InstructionSelector.cpp
    LiveInterval.cpp
    RegisterAllocator.cpp
    PrologueEpilogueInserter.cpp
    PassManager.cpp
)
target_link_libraries(AuroraCodeGen PUBLIC AuroraTarget AuroraX86)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraCodeGen_shared SHARED
        MachineInstr.cpp
        MachineBasicBlock.cpp
        MachineFunction.cpp
        SelectionDAG.cpp
        InstructionSelector.cpp
        LiveInterval.cpp
        RegisterAllocator.cpp
        PrologueEpilogueInserter.cpp
        PassManager.cpp
    )
    target_link_libraries(AuroraCodeGen_shared PUBLIC AuroraTarget AuroraX86)
    set_target_properties(AuroraCodeGen_shared PROPERTIES OUTPUT_NAME AuroraCodeGen)
endif()

install(TARGETS AuroraCodeGen DESTINATION lib)
