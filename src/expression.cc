#include "expression.h"

//---------------------------------------------------------------------------
// MiniParse::Expression::Binary
//---------------------------------------------------------------------------
namespace MiniParse::Expression
{
void Binary::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
//---------------------------------------------------------------------------
// MiniParse::Expression::Grouping
//---------------------------------------------------------------------------
void Grouping::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}

//---------------------------------------------------------------------------
// MiniParse::Expression::Literal
//---------------------------------------------------------------------------
void Literal::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}

//---------------------------------------------------------------------------
// MiniParse::Expression::Unary
//---------------------------------------------------------------------------
void Unary::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
}