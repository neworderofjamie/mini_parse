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

typedef std::unique_ptr<Base const> ExpressionPtr;

//---------------------------------------------------------------------------
// MiniParse::Expression::Assignment
//---------------------------------------------------------------------------
class Assignment : public Base
{
public:
    Assignment(Token varName, Token::Type op, ExpressionPtr value)
    :  m_VarName(varName), m_Operator(op), m_Value(std::move(value))
    {}

    virtual void accept(Visitor &visitor) const override;

    const Token &getVarName() const { return m_VarName; }
    const Token::Type &getOperator() const { return m_Operator; }
    const Base *getValue() const { return m_Value.get(); }

private:
    const Token m_VarName;
    const Token::Type m_Operator;
    const ExpressionPtr m_Value;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Binary
//---------------------------------------------------------------------------
class Binary : public Base
{
public:
    Binary(ExpressionPtr left, Token op, ExpressionPtr right)
    :  m_Left(std::move(left)), m_Operator(op), m_Right(std::move(right))
    {}

    virtual void accept(Visitor &visitor) const override;

    const Base *getLeft() const { return m_Left.get(); }
    const Token &getOperator() const { return m_Operator; }
    const Base *getRight() const { return m_Right.get(); }

private:
    const ExpressionPtr m_Left;
    const Token m_Operator;
    const ExpressionPtr m_Right;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Grouping
//---------------------------------------------------------------------------
class Grouping : public Base
{
public:
    Grouping(ExpressionPtr expression)
    :  m_Expression(std::move(expression))
    {}

    virtual void accept(Visitor &visitor) const override;

    const Base *getExpression() const { return m_Expression.get(); }

private:
    const ExpressionPtr m_Expression;
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
// MiniParse::Expression::Logical
//---------------------------------------------------------------------------
class Logical : public Base
{
public:
    Logical(ExpressionPtr left, Token op, ExpressionPtr right)
    :  m_Left(std::move(left)), m_Operator(op), m_Right(std::move(right))
    {}

    virtual void accept(Visitor &visitor) const override;

    const Base *getLeft() const { return m_Left.get(); }
    const Token &getOperator() const { return m_Operator; }
    const Base *getRight() const { return m_Right.get(); }

private:
    const ExpressionPtr m_Left;
    const Token m_Operator;
    const ExpressionPtr m_Right;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Variable
//---------------------------------------------------------------------------
class Variable : public Base
{
public:
    Variable(Token name)
    :  m_Name(name)
    {}

    virtual void accept(Visitor &visitor) const override;

   const Token &getName() const { return m_Name; }

private:
    const Token m_Name;
};

//---------------------------------------------------------------------------
// MiniParse::Expression::Unary
//---------------------------------------------------------------------------
class Unary : public Base
{
public:
    Unary(Token op, ExpressionPtr right)
    :  m_Operator(op), m_Right(std::move(right))
    {}

    virtual void accept(Visitor &visitor) const override;

    const Token &getOperator() const { return m_Operator; }
    const Base *getRight() const { return m_Right.get(); }

private:
    const Token m_Operator;
    const ExpressionPtr m_Right;
};


//---------------------------------------------------------------------------
// MiniParse::Expression::Visitor
//---------------------------------------------------------------------------
class Visitor
{
public:
    virtual void visit(const Assignment &assignement) = 0;
    virtual void visit(const Binary &binary) = 0;
    virtual void visit(const Grouping &grouping) = 0;
    virtual void visit(const Literal &literal) = 0;
    virtual void visit(const Logical &logical) = 0;
    virtual void visit(const Variable &variable) = 0;
    virtual void visit(const Unary &unary) = 0;
};
}   // namespace MiniParse::Expression