#include "interpreter.h"

// Standard C++ includes
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>
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
void Interpreter::interpret(const std::vector<std::unique_ptr<const Statement::Base>> &statements)
{
    for(auto &s : statements) {
        s.get()->accept(*this);
    }
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
void Interpreter::visit(const Expression::Variable &variable)
{
    m_Value = m_Environment.get(variable.getName());
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
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Expression &expression)
{
    evaluate(expression.getExpression());
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::VarDeclaration &varDeclaration)
{
    Token::LiteralValue value;

    if(varDeclaration.getInitialiser() != nullptr) {
        evaluate(varDeclaration.getInitialiser() );
        value = m_Value;
    }

    m_Environment.define(varDeclaration.getName(), value);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Print &print)
{
#define PRINT(TYPE) [](TYPE x){ std::cout << "("#TYPE")" << x << std::endl; }

    auto value = evaluate(print.getExpression());
    std::visit(
        MiniParse::Utils::Overload{
            PRINT(bool),
            PRINT(float),
            PRINT(double),
            PRINT(uint32_t),
            PRINT(int32_t),
            PRINT(uint64_t),
            PRINT(int64_t),
            [](std::monostate) { std::cout << "invalid"; }},
        value);

#undef PRINT
}
//---------------------------------------------------------------------------
// MiniParse::Interpreter::Environment
//---------------------------------------------------------------------------
void Interpreter::Environment::define(const Token &name, Token::LiteralValue value)
{
    if(!m_Values.try_emplace(name.lexeme, value).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
}
//---------------------------------------------------------------------------
Token::LiteralValue Interpreter::Environment::get(const Token &name) const
{
    auto val = m_Values.find(std::string{name.lexeme});
    if(val == m_Values.end()) {
        throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
    else {
        return val->second;
    }
}
}   // namespace MiniParse