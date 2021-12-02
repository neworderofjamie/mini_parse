#include "expression.h"

//---------------------------------------------------------------------------
// Parser::Expression::Binary
//---------------------------------------------------------------------------
namespace Parser::Expression
{
void Binary::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
//---------------------------------------------------------------------------
// Parser::Expression::Grouping
//---------------------------------------------------------------------------
void Grouping::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}

//---------------------------------------------------------------------------
// Parser::Expression::Literal
//---------------------------------------------------------------------------
void Literal::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}

//---------------------------------------------------------------------------
// Parser::Expression::Unary
//---------------------------------------------------------------------------
void Unary::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
}