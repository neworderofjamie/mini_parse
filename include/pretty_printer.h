#pragma once

// Standard C++ includes
#include <initializer_list>
#include <sstream>

// Mini-parse includes
#include "expression.h"

//---------------------------------------------------------------------------
// Parser::PrettyPrinter
//---------------------------------------------------------------------------
namespace Parser
{
class PrettyPrinter : public Expression::Visitor
{
public:
    std::string print(const Expression::Base &expression);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::Binary &binary) override;
    virtual void visit(const Expression::Grouping &grouping) override;
    virtual void visit(const Expression::Literal &literal) override;
    virtual void visit(const Expression::Unary &unary) override;

private:
    // Private methods
    void parenthesize(std::string_view name, std::initializer_list<const Expression::Base *> expressions);

    // Members
    std::ostringstream m_StringStream;
};
}