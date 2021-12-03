#include "interpreter.h"

// Standard C++ includes
#include <limits>
#include <stdexcept>
#include <type_traits>

// Standard C includes
#include <cassert>

// Mini-parse includes
#include "utils.h"

//---------------------------------------------------------------------------
// MiniParse::Interpreter
//---------------------------------------------------------------------------
namespace MiniParse
{
Token::LiteralValue Interpreter::evaluate(const Expression::Base *expression)
{
    expression->accept(*this);
    return m_Value;
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Binary &binary)
{
    auto leftValue = evaluate(binary.getLeft());
    auto rightValue = evaluate(binary.getRight());

    const auto opType = binary.getOperator().type;

    m_Value = std::visit(
        Utils::Overload{
            [opType](auto left, auto right)
            { 
                typedef typename std::common_type<decltype(left), decltype(right)>::type R;
                const R comLeft = static_cast<R>(left);
                const R comRight = static_cast<R>(right);
                if(opType == Token::Type::PLUS) {
                    return MiniParse::Token::LiteralValue(comLeft + comRight);
                }
                else if(opType == Token::Type::MINUS) {
                    return MiniParse::Token::LiteralValue(comLeft - comRight);
                }
                else if(opType == Token::Type::STAR) {
                    return MiniParse::Token::LiteralValue(comLeft * comRight);
                }
                else if(opType == Token::Type::SLASH) {
                    return MiniParse::Token::LiteralValue(comLeft / comRight);
                }
                else if(opType == Token::Type::GREATER) {
                    return MiniParse::Token::LiteralValue(comLeft > comRight);
                }
                 else if(opType == Token::Type::GREATER_EQUAL) {
                    return MiniParse::Token::LiteralValue(comLeft >= comRight);
                }
                else if(opType == Token::Type::LESS) {
                    return MiniParse::Token::LiteralValue(comLeft < comRight);
                }
                else if(opType == Token::Type::LESS_EQUAL) {
                    return MiniParse::Token::LiteralValue(comLeft < comRight);
                }
                else if(opType == Token::Type::NOT_EQUAL) {
                    return MiniParse::Token::LiteralValue(comLeft != comRight);
                }
                else if(opType == Token::Type::EQUAL_EQUAL) {
                    return MiniParse::Token::LiteralValue(comLeft == comRight);
                }
                else {
                    throw std::runtime_error("Unsupported binary operation");
                }
            },
            [](std::monostate, std::monostate) { return MiniParse::Token::LiteralValue(); },
            [](std::monostate, auto) { return MiniParse::Token::LiteralValue(); },
            [](auto, std::monostate) { return MiniParse::Token::LiteralValue(); }},
        leftValue, rightValue);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Grouping &grouping)
{
    evaluate(grouping.getExpression());
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Literal &literal)
{
    m_Value = literal.getValue();
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Unary &unary)
{
    assert(false);

    if(unary.getOperator().type == Token::Type::PLUS) {
        // **TODO** Some sort of type promotion happens here
        //m_Value = rightValue;
    }
    /*else if(unary.getOperator().type == Token::Type::MINUS) {
        m_Value = std::visit(
            overload{
                [](auto x) { return -x; },
                [](std::monostate) { return Token::LiteralValue(); }},
                rightValue);
    }
    else if(unary.getOperator().type == Token::Type::TILDA) {
        m_Value = std::visit(
            [](auto x)
            {
                if constexpr(std::is_integral_v<decltype(x)>) {
                    return ~x;
                }
                else {
                    return Token::LiteralValue();
                }
            }, 
            rightValue);
    }*/
    else if(unary.getOperator().type == Token::Type::NOT) {
        /*m_Value = std::visit(
            overload{
                [](auto x) { return !static_cast<bool>(x); },
                [](std::monostate) { return Token::LiteralValue(); }},
                rightValue);*/
    }

}
}   // namespace MiniParse