add_library(AuroraADT
    SmallVector.cpp
    BitVector.cpp
    Graph.cpp
    Allocator.cpp
    SparseSet.cpp
)
target_link_libraries(AuroraADT PUBLIC AuroraCommon)

if(AURORA_BUILD_SHARED_LIBS)
    add_library(AuroraADT_shared SHARED
        SmallVector.cpp
        BitVector.cpp
        Graph.cpp
        Allocator.cpp
        SparseSet.cpp
    )
    target_link_libraries(AuroraADT_shared PUBLIC AuroraCommon)
    set_target_properties(AuroraADT_shared PROPERTIES OUTPUT_NAME AuroraADT)
endif()

install(TARGETS AuroraADT DESTINATION lib)
