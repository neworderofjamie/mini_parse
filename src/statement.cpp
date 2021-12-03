#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::Statement::Expression
//---------------------------------------------------------------------------
namespace MiniParse::Statement
{
void Expression::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
//---------------------------------------------------------------------------
void Print::accept(Visitor &visitor) const
{
    visitor.visit(*this);
}
}   // namespace MiniParse::Statement