# MiniC Frontend Layout

MiniC is organized as a small compiler frontend library with public headers under `include/minic` and implementation files under `src`. The `minic` CLI is a thin executable that links the `MiniCFrontend` library and the Aurora backend/MC stack.

- `include/minic/Lexer.h` and `src/Lexer.cpp`: tokenization and keyword/operator recognition.
- `include/minic/AST.h`: syntax tree and MiniC type model.
- `include/minic/ASTUtils.h` and `src/ASTUtils.cpp`: shared AST/type predicates that are independent of parser and codegen state.
- `include/minic/Parser.h` and `src/Parser.cpp`: parser facade, token cursor, program/function entry points.
- `src/ParserTypes.cpp`: type specifiers, declarators, typedefs, enums, static assertions, parameter lists, and constant expressions.
- `src/ParserStmts.cpp`: blocks, declarations, control-flow statements, and initializers.
- `src/ParserExpr.cpp`: expression precedence parser and primary/postfix/unary parsing.
- `include/minic/CodeGen.h` and `src/CodeGen.cpp`: codegen facade, AIR type conversion, function emission, scope state, and control helpers.
- `src/CodeGenDecl.cpp`: local declarations and initializer lowering.
- `src/CodeGenStmts.cpp`: statement and control-flow lowering.
- `src/CodeGenExpr.cpp`: expression, lvalue, call, and short-circuit lowering.
- `src/CodeGenAnalysis.cpp`: expression analysis, constant evaluation, type inference, and pointer/unsigned arithmetic helpers.

Keep new language features in the narrowest owning layer first. Add shared helpers to `ASTUtils.*` only when both parsing and codegen need them. Include local headers through `minic/...` paths so source layout stays decoupled from header layout.
