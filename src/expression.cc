#include "expression.h"

#define IMPLEMENT_ACCEPT(CLASS_NAME)                                        \
    void MiniParse::Expression::CLASS_NAME::accept(Visitor &visitor) const  \
    {                                                                       \
        visitor.visit(*this);                                               \
    }


IMPLEMENT_ACCEPT(Assignment)
IMPLEMENT_ACCEPT(Binary)
IMPLEMENT_ACCEPT(Grouping)
IMPLEMENT_ACCEPT(Literal)
IMPLEMENT_ACCEPT(Logical)
IMPLEMENT_ACCEPT(Variable)
IMPLEMENT_ACCEPT(Unary)