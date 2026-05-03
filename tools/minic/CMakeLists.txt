add_executable(minic
    main.cpp
    Lexer.cpp
    Parser.cpp
    CodeGen.cpp
)

target_include_directories(minic PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(minic PRIVATE
    AuroraCommon
    AuroraAir
    AuroraMC
    AuroraCodeGen
    AuroraX86
    AuroraTarget
)
