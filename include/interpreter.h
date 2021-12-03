#pragma once

// Mini-parse includes
#include "expression.h"

//---------------------------------------------------------------------------
// MiniParse::PrettyPrinter
//---------------------------------------------------------------------------
namespace MiniParse
{
class Interpreter : public Expression::Visitor
{
public:
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    Token::LiteralValue evaluate(const Expression::Base *expression);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Unary &unary) override;

private:
    Token::LiteralValue m_Value;
};
}