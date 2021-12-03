#include "pretty_printer.h"

// Mini-parse includes
#include "utils.h"

//---------------------------------------------------------------------------
// MiniParse::PrettyPrinter
//---------------------------------------------------------------------------
namespace MiniParse
{
std::string PrettyPrinter::print(const Expression::Base &expression)
{
    // Clear string stream
    m_StringStream.str("");

    // Visit parseExpression
    expression.accept(*this);

    // Return string stream contents
    return m_StringStream.str();
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Binary &binary)
{
    parenthesize(binary.getOperator().lexeme,
                 {binary.getLeft(), binary.getRight()});
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Grouping &grouping)
{
    parenthesize("group",
                 {grouping.getExpression()});
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Literal &literal)
{
    std::visit(
        Utils::Overload{
            [this](auto x) { m_StringStream << x; },
            [this](std::monostate) {m_StringStream << "invalid"; }},
        literal.getValue());
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Unary &unary)
{
    parenthesize(unary.getOperator().lexeme, {unary.getRight()});
}
//---------------------------------------------------------------------------
void PrettyPrinter::parenthesize(std::string_view name, std::initializer_list<const Expression::Base *> expressions)
{
    m_StringStream << "(" << name;

    for(auto e : expressions) {
        m_StringStream << " ";
        e->accept(*this);
    }

    m_StringStream << ")";
}
}
