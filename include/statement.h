#pragma once

// Standard C++ includes
#include <memory>
#include <vector>

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

typedef std::unique_ptr<Base const> StatementPtr;
typedef std::vector<StatementPtr> StatementList;

//---------------------------------------------------------------------------
// MiniParse::Statement::Compound
//---------------------------------------------------------------------------
class Compound : public Base
{
public:
    Compound(StatementList statements)
    :  m_Statements(std::move(statements))
    {}

    virtual void accept(Visitor &visitor) const override;

    const StatementList &getStatements() const { return m_Statements; }

private:
    const StatementList m_Statements;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::Expression
//---------------------------------------------------------------------------
class Expression : public Base
{
public:
    Expression(MiniParse::Expression::ExpressionPtr expression)
    :  m_Expression(std::move(expression))
    {}

    virtual void accept(Visitor &visitor) const override;

    const MiniParse::Expression::Base *getExpression() const { return m_Expression.get(); }

private:
    const MiniParse::Expression::ExpressionPtr m_Expression;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::VarDeclaration
//---------------------------------------------------------------------------
class VarDeclaration : public Base
{
public:
    typedef std::vector<std::tuple<Token, MiniParse::Expression::ExpressionPtr>> InitDeclaratorList;

    VarDeclaration(std::vector<Token> declarationSpecifiers, InitDeclaratorList initDeclaratorList)
    :   m_DeclarationSpecifiers(std::move(declarationSpecifiers)), m_InitDeclaratorList(std::move(initDeclaratorList))
    {}

    virtual void accept(Visitor &visitor) const override;

    const std::vector<Token> &getDeclarationSpecifiers() const { return m_DeclarationSpecifiers; }
    const InitDeclaratorList &getInitDeclaratorList() const { return m_InitDeclaratorList; }
    
private:
    const std::vector<Token> m_DeclarationSpecifiers;
    const InitDeclaratorList m_InitDeclaratorList;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::Print
//---------------------------------------------------------------------------
// **HACK** temporary until function calling is working
class Print : public Base
{
public:
    Print(MiniParse::Expression::ExpressionPtr expression)
    :  m_Expression(std::move(expression))
    {}

    virtual void accept(Visitor &visitor) const override;

    const MiniParse::Expression::Base *getExpression() const { return m_Expression.get(); }

private:
    const MiniParse::Expression::ExpressionPtr m_Expression;
};

//---------------------------------------------------------------------------
// MiniParse::Statement::Visitor
//---------------------------------------------------------------------------
class Visitor
{
public:
    virtual void visit(const Compound& compound) = 0;
    virtual void visit(const Expression &expression) = 0;
    virtual void visit(const VarDeclaration &varDeclaration) = 0;
    virtual void visit(const Print &print) = 0;
};
}   // namespace MiniParse::Statement