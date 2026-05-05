add_library(MiniCFrontend STATIC
    src/ASTUtils.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/ParserTypes.cpp
    src/ParserStmts.cpp
    src/ParserExpr.cpp
    src/CodeGen.cpp
    src/CodeGenDecl.cpp
    src/CodeGenStmts.cpp
    src/CodeGenExpr.cpp
    src/CodeGenAnalysis.cpp
)

target_include_directories(MiniCFrontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(MiniCFrontend PUBLIC
    AuroraAir
)

add_executable(minic
    src/main.cpp
)

target_link_libraries(minic PRIVATE
    MiniCFrontend
    AuroraMC
)
