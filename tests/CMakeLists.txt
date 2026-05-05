add_library(AuroraTestCommon INTERFACE)
target_link_libraries(AuroraTestCommon INTERFACE
    pthread
    GTest::gtest
    GTest::gtest_main
)

add_executable(ADT_tests
    ADT/SmallVectorTest.cpp
    ADT/BitVectorTest.cpp
    ADT/GraphTest.cpp
    ADT/AllocatorTest.cpp
    ADT/SparseSetTest.cpp
)
target_link_libraries(ADT_tests PRIVATE AuroraADT AuroraTestCommon)
add_test(NAME ADT_tests COMMAND ADT_tests)

add_executable(Air_tests
    Air/TypeTest.cpp
    Air/InstructionTest.cpp
    Air/BasicBlockTest.cpp
    Air/FunctionTest.cpp
    Air/BuilderTest.cpp
    Air/ConstantTest.cpp
    Air/ModuleTest.cpp
    Air/TypeCoverageTest.cpp
    Air/InstructionFactoryTest.cpp
    Air/ToStringCoverageTest.cpp
)
target_link_libraries(Air_tests PRIVATE AuroraAir AuroraTestCommon)
add_test(NAME Air_tests COMMAND Air_tests)

add_executable(Target_tests
    Target/X86RegisterInfoTest.cpp
    Target/X86InstrInfoTest.cpp
    Target/X86ISelPatternsTest.cpp
    Target/X86CallingConvTest.cpp
    Target/X86InstrEncodeTest.cpp
    Target/AArch64TargetTest.cpp
    Target/FuncCoverTest.cpp
)
target_link_libraries(Target_tests PRIVATE AuroraCodeGen AuroraTestCommon)
add_test(NAME Target_tests COMMAND Target_tests)

add_executable(CodeGen_tests
    CodeGen/MachineInstrTest.cpp
    CodeGen/SelectionDAGTest.cpp
    CodeGen/SelectionDAGFullTest.cpp
    CodeGen/InstructionSelectorTest.cpp
    CodeGen/PassManagerTest.cpp
    CodeGen/PassManagerDeepCoverageTest.cpp
    CodeGen/LiveIntervalTest.cpp
    CodeGen/ISelCoverageTest.cpp
    CodeGen/ISelPipeline2Test.cpp
    CodeGen/ISelRefactorTest.cpp
    CodeGen/FuncCoverTest.cpp
    CodeGen/PipelineBoostTest.cpp
    CodeGen/FinalCoverageTest.cpp
)
target_link_libraries(CodeGen_tests PRIVATE AuroraCodeGen AuroraTestCommon)
add_test(NAME CodeGen_tests COMMAND CodeGen_tests)

add_executable(MC_tests
    MC/AsmPrinterTest.cpp
    MC/IntegrationTest.cpp
    MC/IntegrationFullTest.cpp
    MC/CoverageBoostTest.cpp
    MC/ObjectWriterTest.cpp
    MC/ObjectWriterFullTest.cpp
    MC/ObjWriterFuncTest.cpp
    MC/X86ObjectEncoderTest.cpp
    MC/AArch64AsmPrinterTest.cpp
)
target_link_libraries(MC_tests PRIVATE AuroraMC AuroraTestCommon)
add_test(NAME MC_tests COMMAND MC_tests)

add_executable(MiniC_tests
    MiniC/GlobalTest.cpp
    MiniC/AlignofTest.cpp
    MiniC/AlignasTest.cpp
    MiniC/StructTest.cpp
    MiniC/UnionTest.cpp
    MiniC/FeatureCoverageTest.cpp
    MiniC/DesignatedInitializerTest.cpp
    MiniC/CompoundLiteralTest.cpp
)
target_link_libraries(MiniC_tests PRIVATE MiniCFrontend AuroraTestCommon)
add_test(NAME MiniC_tests COMMAND MiniC_tests)

# ---- Benchmarks ----
if(AURORA_BUILD_BENCHMARKS)
    add_library(AuroraBenchCommon INTERFACE)
    target_link_libraries(AuroraBenchCommon INTERFACE benchmark::benchmark benchmark::benchmark_main)

    add_executable(AuroraBench
        Bench/BenchISel.cpp
        Bench/BenchRA.cpp
        Bench/BenchADT.cpp
    )
    target_link_libraries(AuroraBench PRIVATE AuroraMC AuroraBenchCommon)
endif()
