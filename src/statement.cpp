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
}