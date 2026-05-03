add_library(AuroraAir STATIC
    Type.cpp
    Constant.cpp
    Instruction.cpp
    BasicBlock.cpp
    Function.cpp
    Module.cpp
    Builder.cpp
)
target_link_libraries(AuroraAir PUBLIC AuroraADT)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraAir_shared SHARED
        Type.cpp
        Constant.cpp
        Instruction.cpp
        BasicBlock.cpp
        Function.cpp
        Module.cpp
        Builder.cpp
    )
    target_link_libraries(AuroraAir_shared PUBLIC AuroraADT)
    set_target_properties(AuroraAir_shared PROPERTIES OUTPUT_NAME AuroraAir)
endif()

install(TARGETS AuroraAir DESTINATION lib)
