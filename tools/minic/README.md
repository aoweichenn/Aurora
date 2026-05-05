# MiniC Frontend Layout

MiniC is organized as a small compiler frontend library with domain-scoped public headers under `include/minic` and matching implementation directories under `src`. The `minic` CLI is a thin executable that links the `MiniCFrontend` library and the Aurora backend/MC stack.

- `include/minic/ast/` and `src/ast/`: syntax tree types, AST umbrellas, and shared AST/type predicates.
- `include/minic/lex/` and `src/lex/`: token definitions, token names, tokenization, and keyword/operator recognition.
- `include/minic/parse/` and `src/parse/`: parser facade, token cursor, type/declaration parsing, statements, initializers, and expression precedence parsing.
- `include/minic/codegen/` and `src/codegen/`: codegen facade, AIR type conversion, function emission, declarations, statements, expressions, and analysis helpers.
- `src/driver/`: command-line entry point and integration with the Aurora backend/MC stack.

Keep new language features in the narrowest owning layer first. Add shared helpers to `ast/ASTUtils.*` only when both parsing and codegen need them. Include local headers through domain paths like `minic/parse/Parser.h` so source layout stays decoupled from header layout.
