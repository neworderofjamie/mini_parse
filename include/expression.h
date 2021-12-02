#pragma once

// Standard C++ includes
#include <memory>

// Mini-parse includes
#include "token.h"

// Forward declarations
namespace MiniParse::Expression 
{
class Visitor;
}

//---------------------------------------------------------------------------
// MiniParse::Expression::Base
//---------------------------------------------------------------------------
namespace MiniParse::Expression
{
struct Base
{
    virtual void accept(Visitor &visitor) const = 0;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Binary
//---------------------------------------------------------------------------
class Binary : public Base
{
public:
    Binary(const Base *left, const Token &op, const Base *right)
    :  m_Left(left), m_Operator(op), m_Right(right)
    {}

    virtual void accept(Visitor &visitor) const override;

    const Base *getLeft() const { return m_Left.get(); }
    const Token &getOperator() const { return m_Operator; }
    const Base *getRight() const { return m_Right.get(); }

private:
    const std::unique_ptr<const Base> m_Left;
    const Token &m_Operator;
    const std::unique_ptr<const Base> m_Right;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Grouping
//---------------------------------------------------------------------------
class Grouping : public Base
{
public:
    Grouping(const Base *expression)
    :  m_Expression(expression)
    {}

    virtual void accept(Visitor &visitor) const override;

    const Base *getExpression() const { return m_Expression.get(); }

private:
    const std::unique_ptr<const Base> m_Expression;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Literal
//---------------------------------------------------------------------------
class Literal : public Base
{
public:
    Literal(Token::LiteralValue value)
    :  m_Value(value)
    {}

    virtual void accept(Visitor &visitor) const override;

    Token::LiteralValue getValue() const { return m_Value; }

private:
    const Token::LiteralValue m_Value;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Unary
//---------------------------------------------------------------------------
class Unary : public Base
{
public:
    Unary(const Token &op, const Base *right)
    :  m_Operator(op), m_Right(right)
    {}

    virtual void accept(Visitor &visitor) const override;

    const Token &getOperator() const { return m_Operator; }
    const Base *getRight() const { return m_Right.get(); }

private:
    const Token &m_Operator;
    const std::unique_ptr<const Base> m_Right;
};


//---------------------------------------------------------------------------
// MiniParse::Expression::Visitor
//---------------------------------------------------------------------------
class Visitor
{
public:
    virtual void visit(const Binary &binary) = 0;
    virtual void visit(const Grouping &grouping) = 0;
    virtual void visit(const Literal &literal) = 0;
    virtual void visit(const Unary &unary) = 0;
};
}   // namespace MiniParse::Expression