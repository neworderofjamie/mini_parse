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
// MiniParse::Statement::Expression
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
// MiniParse::Statement::VarDeclaration
//---------------------------------------------------------------------------
class VarDeclaration : public Base
{
public:
    VarDeclaration(Token type, Token name, const MiniParse::Expression::Base *initialiser)
    :   m_Type(type), m_Name(name), m_Initializer(initialiser)
    {}

    virtual void accept(Visitor &visitor) const override;

    const Token &getType() const { return m_Type;  }
    const Token &getName() const { return m_Name; }
    const MiniParse::Expression::Base *getInitialiser() const { return m_Initializer.get(); }

private:
    const Token m_Type;
    const Token m_Name;
    const std::unique_ptr<const MiniParse::Expression::Base> m_Initializer;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::Print
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
    virtual void visit(const VarDeclaration &varDeclaration) = 0;
    virtual void visit(const Print &print) = 0;
};
}   // namespace MiniParse::Statement