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
    // Public API
    //---------------------------------------------------------------------------
    Token::LiteralValue evaluate(const Expression::Base *expression);
    void interpret(const std::vector<std::unique_ptr<const Statement::Base>> &statements);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Assignment &assignement) override;
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Variable &variable) override;
    virtual void visit(const Expression::Unary &unary) override;
    
    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Expression &expression) override;
    virtual void visit(const Statement::VarDeclaration &varDeclaration) override;
    virtual void visit(const Statement::Print &print) override;

private:
    //---------------------------------------------------------------------------
    // MiniParse::Interpreter::Environment
    //---------------------------------------------------------------------------
    class Environment
    {
    public:
        // **TODO** type
        void define(const Token &name, Token::LiteralValue value);

        // **TODO** type
        void assign(const Token &name, Token::LiteralValue value, Token::Type op);

        // **TODO** type
        Token::LiteralValue get(const Token &name) const;

    private:
        std::unordered_map<std::string_view, Token::LiteralValue> m_Values;
    };

    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Token::LiteralValue m_Value;
    
    Environment m_Environment;
};
}