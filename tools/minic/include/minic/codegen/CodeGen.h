#pragma once

#include "minic/ast/AST.h"
#include "Aurora/Air/Module.h"
#include "Aurora/Air/Builder.h"
#include "Aurora/Air/Type.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace aurora {
class Constant;
class GlobalVariable;
}

namespace minic {

class CodeGen {
public:
    explicit CodeGen();
    std::unique_ptr<aurora::Module> generate(const Program& program);

private:
    struct Variable {
        CType type;
        unsigned pointerVReg;
    };

    struct LValue {
        CType type;
        unsigned pointerVReg;
    };

    struct Global {
        CType type;
        aurora::GlobalVariable* variable;
        std::string name;
        bool isExtern;
    };

    std::unique_ptr<aurora::Module> module_;
    aurora::AIRBuilder* builder_;
    std::unordered_map<std::string, aurora::Function*> functionMap_;
    std::unordered_map<std::string, CType> functionReturnTypes_;
    std::unordered_map<std::string, Global> globals_;
    std::vector<std::unordered_map<std::string, Variable>> scopes_;
    std::vector<aurora::BasicBlock*> breakStack_;
    std::vector<aurora::BasicBlock*> continueStack_;
    CType currentReturnType_;
    unsigned ifCounter_ = 0;
    unsigned loopCounter_ = 0;
    unsigned conditionalCounter_ = 0;
    unsigned switchCounter_ = 0;

    aurora::Type* toAirType(CType type, bool allowVoid = true) const;

    void genStmt(const Stmt& stmt);
    void genBlock(const BlockStmt& block, bool createScope = true);
    void genDeclStmt(const DeclStmt& stmt);
    void genReturnStmt(const ReturnStmt& stmt);
    void genIfStmt(const IfStmt& stmt);
    void genWhileStmt(const WhileStmt& stmt);
    void genDoWhileStmt(const DoWhileStmt& stmt);
    void genForStmt(const ForStmt& stmt);
    void genSwitchStmt(const SwitchStmt& stmt);
    void genExprStmt(const ExprStmt& stmt);
    void genBreakStmt();
    void genContinueStmt();
    void genArrayInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name);
    void genArrayInitializerAtAddress(CType type, unsigned base, const InitListExpr& init, const std::string& name);
    void genStructInitializer(CType type, unsigned pointer, const InitListExpr& init, const std::string& name);
    void genRecordInitializerAtAddress(CType type, unsigned base, const InitListExpr& init, const std::string& name);
    void genInitializerAtAddress(CType type, unsigned address, const Expr& init, const std::string& name);
    void genDesignatedInitializerAtAddress(CType type, unsigned address, const InitListExpr::Designator& designator, size_t partIndex, const Expr& init, const std::string& name);
    void genZeroInitializerAtAddress(CType type, unsigned address);
    unsigned addByteOffset(unsigned base, uint64_t offset);

    unsigned genExpr(const Expr& node);

    unsigned genBinaryExpr(const BinaryExpr& be);
    unsigned genUnaryExpr(const UnaryExpr& ue);
    unsigned genCastExpr(const CastExpr& ce);
    unsigned genAssignExpr(const AssignExpr& ae);
    unsigned genIncDecExpr(const IncDecExpr& ie);
    unsigned genIndexExpr(const IndexExpr& ie);
    unsigned genMemberExpr(const MemberExpr& me);
    unsigned genCompoundLiteralExpr(const CompoundLiteralExpr& cle);
    unsigned genSizeofExpr(const SizeofExpr& se);
    unsigned genAlignofExpr(const AlignofExpr& ae);
    unsigned genCommaExpr(const CommaExpr& ce);
    unsigned genCallExpr(const CallExpr& ce);
    unsigned genConditionalExpr(const ConditionalExpr& ce);
    unsigned genIntLitExpr(const IntLitExpr& ie) const;
    unsigned genVarExpr(const VarExpr& ve);
    unsigned genConditionValue(const Expr& expr);
    bool containsCall(const Expr& expr) const;
    LValue genLValue(const Expr& expr);
    LValue genMemberLValue(const MemberExpr& me);
    unsigned genCompoundLiteralStorage(const CompoundLiteralExpr& cle);
    unsigned genAddressOfVariable(const VarExpr& ve);
    uint64_t sizeOfType(CType type) const;
    uint64_t alignOfType(CType type) const;
    int64_t evalConstantExpr(const Expr& expr) const;
    CType inferExprType(const Expr& expr);
    unsigned scalePointerOffset(CType pointerType, unsigned value);
    unsigned genRemainder(CType lhsType, CType rhsType, unsigned lhs, unsigned rhs);

    Variable& findVariable(const std::string& name);
    Variable* findVariableInScopes(const std::string& name);
    Global& findGlobal(const std::string& name);
    unsigned genGlobalAddress(const std::string& name);
    void declareGlobal(const GlobalDecl& decl);
    aurora::Constant* buildGlobalInitializer(CType type, const Expr& init, const std::string& name);
    aurora::Constant* buildGlobalConstantFromBytes(CType type, const std::vector<uint8_t>& bytes, uint64_t offset);
    void storeGlobalInitializerBytes(CType type, std::vector<uint8_t>& bytes, uint64_t offset, const Expr& init, const std::string& name);
    void storeGlobalDesignatedInitializerBytes(CType type, std::vector<uint8_t>& bytes, uint64_t offset, const InitListExpr::Designator& designator, size_t partIndex, const Expr& init, const std::string& name);
    void declareVariable(const std::string& name, CType type, const Expr* init);
    void pushScope();
    void popScope();
    bool currentBlockTerminated() const;
    void branchToIfOpen(aurora::BasicBlock* target);
};

} // namespace minic
