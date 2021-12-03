#pragma once

// Standard C++ includes
#include <memory>

// Mini-parse includes
#include "expression.h"

// Forward declarations
namespace MiniParse::Statement 
{
class Visitor;
}

//---------------------------------------------------------------------------
// MiniParse::Statement::Base
//---------------------------------------------------------------------------
namespace MiniParse::Statement
{
struct Base
{
    virtual void accept(Visitor &visitor) const = 0;
};


//---------------------------------------------------------------------------
// MiniParse::Expression
//---------------------------------------------------------------------------
class Expression : public Base
{
public:
    Expression(const MiniParse::Expression::Base *expression)
    :  m_Expression(expression)
    {}

    virtual void accept(Visitor &visitor) const override;

    const MiniParse::Expression::Base *getExpression() const { return m_Expression.get(); }

private:
    const std::unique_ptr<const MiniParse::Expression::Base> m_Expression;
};

//---------------------------------------------------------------------------
// MiniParse::Print
//---------------------------------------------------------------------------
// **HACK** temporary until function calling is working
class Print : public Base
{
public:
    Print(const MiniParse::Expression::Base *expression)
    :  m_Expression(expression)
    {}

    virtual void accept(Visitor &visitor) const override;

    const MiniParse::Expression::Base *getExpression() const { return m_Expression.get(); }

private:
    const std::unique_ptr<const MiniParse::Expression::Base> m_Expression;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::Visitor
//---------------------------------------------------------------------------
class Visitor
{
public:
    virtual void visit(const Expression &expression) = 0;
    virtual void visit(const Print &print) = 0;
};
}   // namespace MiniParse::Statement