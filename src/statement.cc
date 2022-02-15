#include "statement.h"

#define IMPLEMENT_ACCEPT(CLASS_NAME)                                        \
    void MiniParse::Statement::CLASS_NAME::accept(Visitor &visitor) const   \
    {                                                                       \
        visitor.visit(*this);                                               \
    }

// Implement accept methods
IMPLEMENT_ACCEPT(Compound)
IMPLEMENT_ACCEPT(Expression)
IMPLEMENT_ACCEPT(If)
IMPLEMENT_ACCEPT(VarDeclaration)
IMPLEMENT_ACCEPT(Print)