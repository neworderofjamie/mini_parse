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
    virtual void visit(const Expression::Assignment &assignement) final;
    virtual void visit(const Expression::Binary &binary) final;
    virtual void visit(const Expression::Call &call) final;
    virtual void visit(const Expression::Cast &cast) final;
    virtual void visit(const Expression::Conditional &conditional) final;
    virtual void visit(const Expression::Grouping &grouping) final;
    virtual void visit(const Expression::Literal &literal) final;
    virtual void visit(const Expression::Logical &logical) final;
    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) final;
    virtual void visit(const Expression::PrefixIncDec &postfixIncDec) final;
    virtual void visit(const Expression::Variable &variable) final;
    virtual void visit(const Expression::Unary &unary) final;
    
    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Break &breakStatement) final;
    virtual void visit(const Statement::Compound &compound) final;
    virtual void visit(const Statement::Continue &continueStatement) final;
    virtual void visit(const Statement::Do &doStatement) final;
    virtual void visit(const Statement::Expression &expression) final;
    virtual void visit(const Statement::For &forStatement) final;
    virtual void visit(const Statement::If &ifStatement) final;
    virtual void visit(const Statement::Labelled &labelled) final;
    virtual void visit(const Statement::Switch &switchStatement) final;
    virtual void visit(const Statement::VarDeclaration &varDeclaration) final;
    virtual void visit(const Statement::While &whileStatement) final;
    virtual void visit(const Statement::Print &print) final;

private:
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    std::ostringstream m_StringStream;
};
}