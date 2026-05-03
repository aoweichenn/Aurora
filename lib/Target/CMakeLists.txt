add_library(AuroraTarget STATIC
    TargetMachine.cpp
    TargetRegisterInfo.cpp
    TargetInstrInfo.cpp
    TargetLowering.cpp
    TargetCallingConv.cpp
    TargetFrameLowering.cpp
)
target_link_libraries(AuroraTarget PUBLIC AuroraAir)

add_subdirectory(X86)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraTarget_shared SHARED
        TargetMachine.cpp
        TargetRegisterInfo.cpp
        TargetInstrInfo.cpp
        TargetLowering.cpp
        TargetCallingConv.cpp
        TargetFrameLowering.cpp
    )
    target_link_libraries(AuroraTarget_shared PUBLIC AuroraAir)
    set_target_properties(AuroraTarget_shared PROPERTIES OUTPUT_NAME AuroraTarget)
endif()

install(TARGETS AuroraTarget DESTINATION lib)
