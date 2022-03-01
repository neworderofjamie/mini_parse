#pragma once

// Standard C++ includes
#include <initializer_list>
#include <sstream>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::PrettyPrinter
//---------------------------------------------------------------------------
namespace MiniParse
{
class PrettyPrinter : public Expression::Visitor, public Statement::Visitor
{
public:
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    std::string print(const Statement::StatementList &statements);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Assignment &assignement) override;
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Call &call) override;
    virtual void visit(const Expression::Cast &cast) override;
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
    virtual void visit(const Statement::Break &breakStatement) override;
    virtual void visit(const Statement::Compound &compound) override;
    virtual void visit(const Statement::Continue &continueStatement) override;
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
    std::ostringstream m_StringStream;
};
}