#pragma once

// Standard C++ includes
#include <stdexcept>
#include <unordered_map>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

// Forward declarations
namespace MiniParse
{
class ErrorHandler;
}
namespace Type
{
class Base;
}

//---------------------------------------------------------------------------
// MiniParse::Interpreter
//---------------------------------------------------------------------------
namespace MiniParse
{
class TypeChecker : public Expression::Visitor, public Statement::Visitor
{
public:
    //---------------------------------------------------------------------------
    // MiniParse::TypeChecker::Environment
    //---------------------------------------------------------------------------
    class Environment
    {
    public:
        Environment(Environment *enclosing = nullptr)
            : m_Enclosing(enclosing)
        {
        }

        template<typename T>
        void define(std::string_view name, bool isConst = false)
        {
            if(!m_Types.try_emplace(name, T::getInstance(), isConst).second) {
                throw std::runtime_error("Redeclaration of '" + std::string{name} + "'");
            }
        }
        void define(const Token &name, const Type::Base *type, bool isConst = false);
        const Type::Base *assign(const Token &name, const Type::Base *type);
        const Type::Base *incDec(const Token &name);
        std::tuple<const Type::Base*, bool> getType(const Token &name) const;

    private:
        Environment *m_Enclosing;
        std::unordered_map<std::string_view, std::tuple<const Type::Base*, bool>> m_Types;
    };

    TypeChecker()
        : m_Environment(nullptr), m_Type(nullptr)
    {
    }
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    void typeCheck(const Statement::StatementList &statements, Environment &environment);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Assignment &assignement) override;
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Call &call) override;
    virtual void visit(const Expression::Cast &cast) override;
    virtual void visit(const Expression::Conditional &conditional) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Logical &logical) override;
    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) override;
    virtual void visit(const Expression::PrefixIncDec &prefixIncDec) override;
    virtual void visit(const Expression::Variable &variable) override;
    virtual void visit(const Expression::Unary &unary) override;

    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Compound &compound) override;
    virtual void visit(const Statement::Do &doStatement) override;
    virtual void visit(const Statement::Expression &expression) override;
    virtual void visit(const Statement::For &forStatement) override;
    virtual void visit(const Statement::If &ifStatement) override;
    virtual void visit(const Statement::VarDeclaration &varDeclaration) override;
    virtual void visit(const Statement::While &whileStatement) override;
    virtual void visit(const Statement::Print &print) override;

private:
    const Type::Base *evaluateType(const Expression::Base *expression);

    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Environment *m_Environment;
    const Type::Base *m_Type;
};
}