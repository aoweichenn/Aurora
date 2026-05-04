add_library(AuroraMC STATIC
    MCStreamer.cpp
    AsmPrinter.cpp
    X86AsmPrinter.cpp
    X86InstEncoder.cpp
    ObjectWriter.cpp
)
target_link_libraries(AuroraMC PUBLIC AuroraCodeGen)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraMC_shared SHARED
        MCStreamer.cpp
        AsmPrinter.cpp
        X86AsmPrinter.cpp
        X86InstEncoder.cpp
        ObjectWriter.cpp
    )
    target_link_libraries(AuroraMC_shared PUBLIC AuroraCodeGen)
    set_target_properties(AuroraMC_shared PROPERTIES OUTPUT_NAME AuroraMC)
endif()

install(TARGETS AuroraMC DESTINATION lib)
