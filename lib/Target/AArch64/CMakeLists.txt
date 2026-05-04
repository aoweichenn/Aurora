add_library(AuroraAArch64 STATIC
    AArch64RegisterInfo.cpp
    AArch64InstrInfo.cpp
    AArch64TargetLowering.cpp
    AArch64CallingConv.cpp
    AArch64FrameLowering.cpp
    AArch64TargetMachine.cpp
)
target_link_libraries(AuroraAArch64 PUBLIC AuroraTarget)

install(TARGETS AuroraAArch64 DESTINATION lib)
