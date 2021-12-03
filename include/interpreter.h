#pragma once

// Standard C++ includes
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::PrettyPrinter
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
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Unary &unary) override;
    
    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Expression &expression) override;
    virtual void visit(const Statement::Print &print) override;

private:

    Token::LiteralValue m_Value;
};
}