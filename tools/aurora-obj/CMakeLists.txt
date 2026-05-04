add_executable(aurora-obj
    main.cpp
)

target_include_directories(aurora-obj PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aurora-obj PRIVATE
    AuroraCommon
    AuroraAir
    AuroraCodeGen
    AuroraMC
    AuroraX86
    AuroraTarget
)
