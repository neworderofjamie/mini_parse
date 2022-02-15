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
// Anonymous namespace
//---------------------------------------------------------------------------
namespace
{
bool isTruthy(MiniParse::Token::LiteralValue value)
{
    return std::visit(
        MiniParse::Utils::Overload{
            [](auto x) { return static_cast<bool>(x); },
            [](std::monostate) { return false; }},
        value);
}
}

//---------------------------------------------------------------------------
// MiniParse::Interpreter::Environment
//---------------------------------------------------------------------------
namespace MiniParse
{
void Interpreter::Environment::define(const Token &name, Token::LiteralValue value)
{
    if(!m_Values.try_emplace(name.lexeme, value).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
}
//---------------------------------------------------------------------------
void Interpreter::Environment::assign(const Token &name, Token::LiteralValue value, Token::Type op)
{
    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            m_Enclosing->assign(name, value, op);
        }
    }
    else {
        variable->second = std::visit(
            Utils::Overload{
                [op](auto variable, auto assign)
                { 
                    typedef typename std::common_type<decltype(variable), decltype(assign)>::type R;
                    const R comVariable = static_cast<R>(variable);
                    const R comAssign = static_cast<R>(assign);
                    if(op == Token::Type::EQUAL) {
                        return MiniParse::Token::LiteralValue(assign);
                    }
                    else if(op == Token::Type::STAR_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable * assign);
                    }
                    else if(op == Token::Type::SLASH_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable / assign);
                    }
                    /*else if(op == Token::Type::PERCENT_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable % assign);
                    }*/
                    else if(op == Token::Type::PLUS_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable + assign);
                    }
                     else if(op == Token::Type::MINUS_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable - assign);
                    }
                    /*else if(op == Token::Type::AMPERSAND_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable & assign);
                    }
                    else if(op == Token::Type::CARET_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable ^ assign);
                    }
                    else if(op == Token::Type::PIPE_EQUAL) {
                        return MiniParse::Token::LiteralValue(variable | assign);
                    }*/
                    else {
                        throw std::runtime_error("Unsupported assignment operation");
                    }
                },
                [](std::monostate, std::monostate) { return MiniParse::Token::LiteralValue(); },
                [op](std::monostate, auto assign) 
                { 
                    if(op == Token::Type::EQUAL) {
                        return MiniParse::Token::LiteralValue(assign);
                    }
                    else {
                        return MiniParse::Token::LiteralValue();
                    }
                },
                [](auto, std::monostate) { return MiniParse::Token::LiteralValue(); }},
            variable->second, value);
    }
}
//---------------------------------------------------------------------------
Token::LiteralValue Interpreter::Environment::get(const Token &name) const
{
    auto val = m_Values.find(std::string{name.lexeme});
    if(val == m_Values.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->get(name);
        }
    }
    else {
        return val->second;
    }
}

//---------------------------------------------------------------------------
// MiniParse::Interpreter
//---------------------------------------------------------------------------
Token::LiteralValue Interpreter::evaluate(const Expression::Base *expression)
{
    expression->accept(*this);
    return m_Value;
}
//---------------------------------------------------------------------------
void Interpreter::interpret(const Statement::StatementList &statements, Environment &environment)
{
    Environment *previous = m_Environment;
    m_Environment = &environment;
    for(auto &s : statements) {
        s.get()->accept(*this);
    }
    m_Environment = previous;
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Assignment &assignment)
{
    auto value = evaluate(assignment.getValue());
    m_Environment->assign(assignment.getVarName(), value, assignment.getOperator());
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
void Interpreter::visit(const Expression::Logical &logical)
{
    auto leftValue = evaluate(logical.getLeft());

    if(logical.getOperator().type == Token::Type::PIPE_PIPE) {
        if(isTruthy(leftValue)) {
            m_Value = 1;
            return;
        }
    }
    else {
        if(!isTruthy(leftValue)) {
            m_Value = 0;
            return;
        }
    }

    m_Value = (int)isTruthy(evaluate(logical.getRight()));
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Variable &variable)
{
    m_Value = m_Environment->get(variable.getName());
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
void Interpreter::visit(const Statement::Compound &compound)
{
    Environment environment(m_Environment);
    interpret(compound.getStatements(), environment);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Expression &expression)
{
    evaluate(expression.getExpression());
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::If &ifStatement)
{
    if(isTruthy(evaluate(ifStatement.getCondition()))) {
        ifStatement.getThenBranch()->accept(*this);
    }
    else if(ifStatement.getElseBranch() != nullptr) {
        ifStatement.getElseBranch()->accept(*this);
    }
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::VarDeclaration &varDeclaration)
{
    // **TODO** something with type
    for(const auto &var : varDeclaration.getInitDeclaratorList()) {
        Token::LiteralValue value;
        if(std::get<1>(var) != nullptr) {
            evaluate(std::get<1>(var).get());
            value = m_Value;
        }
        m_Environment->define(std::get<0>(var), value);
    }
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
}   // namespace MiniParse