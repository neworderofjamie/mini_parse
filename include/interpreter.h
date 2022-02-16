#pragma once

// Standard C++ includes
#include <list>
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
        void assign(const Token &name, Token::LiteralValue value, Token::Type op);

        // **TODO** type
        Token::LiteralValue get(const Token &name) const;

    private:
        Environment *m_Enclosing;
        std::unordered_map<std::string_view, Token::LiteralValue> m_Values;
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
    virtual void visit(const Expression::Conditional &conditional) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Logical &logical) override;
    virtual void visit(const Expression::Variable &variable) override;
    virtual void visit(const Expression::Unary &unary) override;
    
    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Compound &compound) override;
    virtual void visit(const Statement::Do &doStatement) override;
    virtual void visit(const Statement::Expression &expression) override;
    virtual void visit(const Statement::If &ifStatement) override;
    virtual void visit(const Statement::VarDeclaration &varDeclaration) override;
    virtual void visit(const Statement::While &whileStatement) override;
    virtual void visit(const Statement::Print &print) override;

private:
   

    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Token::LiteralValue m_Value;
    
    Environment *m_Environment;
};
}