#pragma once

// Standard C++ includes
#include <unordered_map>
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::Interpreter
//---------------------------------------------------------------------------
namespace MiniParse
{
class Interpreter : public Expression::Visitor, public Statement::Visitor
{
public:
    //---------------------------------------------------------------------------
    // MiniParse::Interpreter::Callable
    //---------------------------------------------------------------------------
    class Callable
    {
    public:
        virtual size_t getArity() const = 0;
        virtual Token::LiteralValue call(const std::vector<Token::LiteralValue> &arguments) = 0;
    };

    typedef std::variant<Token::LiteralValue, std::reference_wrapper<Callable>> Value;

    //---------------------------------------------------------------------------
    // MiniParse::Interpreter::Environment
    //---------------------------------------------------------------------------
    class Environment
    {
    public:
        Environment(Environment *enclosing = nullptr)
        :   m_Enclosing(enclosing)
        {
        }

        // **TODO** type
        void define(const Token &name, Token::LiteralValue value);

        // **TODO** type
        void define(std::string_view name, Callable &callable);

        // **TODO** type
        Value assign(const Token &name, Token::LiteralValue value, Token::Type op);

        // **TODO** type
        Value prefixIncDec(const Token &name, Token::Type op);

        // **TODO** type
        Value postfixIncDec(const Token &name, Token::Type op);

        // **TODO** type
        Value get(const Token &name) const;

    private:
        Environment *m_Enclosing;
        std::unordered_map<std::string_view, Value> m_Values;
    };

    Interpreter()
    :   m_Environment(nullptr)
    {
    }
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    Token::LiteralValue evaluate(const Expression::Base *expression);
    void interpret(const Statement::StatementList &statements, Environment &environment);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Assignment &assignement) override;
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Call &call) override;
    virtual void visit(const Expression::Conditional &conditional) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Logical &logical) override;
    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) override;
    virtual void visit(const Expression::PrefixIncDec &postfixIncDec) override;
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
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Value m_Value;
    
    Environment *m_Environment;
};
}