add_library(AuroraX86 STATIC
    X86RegisterInfo.cpp
    X86InstrInfo.cpp
    X86InstrEncode.cpp
    X86TargetLowering.cpp
    X86CallingConv.cpp
    X86FrameLowering.cpp
    X86TargetMachine.cpp
    X86ISelPatterns.cpp
)
target_link_libraries(AuroraX86 PUBLIC AuroraTarget)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraX86_shared SHARED
        X86RegisterInfo.cpp
        X86InstrInfo.cpp
        X86InstrEncode.cpp
        X86TargetLowering.cpp
        X86CallingConv.cpp
        X86FrameLowering.cpp
        X86TargetMachine.cpp
        X86ISelPatterns.cpp
    )
    target_link_libraries(AuroraX86_shared PUBLIC AuroraTarget)
    set_target_properties(AuroraX86_shared PROPERTIES OUTPUT_NAME AuroraX86)
endif()

install(TARGETS AuroraX86 DESTINATION lib)
