add_library(MiniCFrontend STATIC
    src/ast/ASTUtils.cpp
    src/lex/Token.cpp
    src/lex/Lexer.cpp
    src/parse/Parser.cpp
    src/parse/ParserTypes.cpp
    src/parse/ParserStmts.cpp
    src/parse/ParserExpr.cpp
    src/codegen/CodeGen.cpp
    src/codegen/CodeGenDecl.cpp
    src/codegen/CodeGenStmts.cpp
    src/codegen/CodeGenExpr.cpp
    src/codegen/CodeGenAnalysis.cpp
)

target_include_directories(MiniCFrontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(MiniCFrontend PUBLIC
    AuroraAir
)

add_executable(minic
    src/driver/main.cpp
)

target_link_libraries(minic PRIVATE
    MiniCFrontend
    AuroraMC
)
