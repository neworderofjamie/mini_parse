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
void Interpreter::Environment::define(std::string_view name, Callable &callable)
{
    if(!m_Values.try_emplace(name, callable).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name});
    }
}
//---------------------------------------------------------------------------
Interpreter::Value Interpreter::Environment::assign(const Token &name, Token::LiteralValue value, Token::Type op)
{
#ifdef _WIN32
    #pragma warning(push)
    #pragma warning(disable: 4804)  // unsafe use of type 'bool' in operation
    #pragma warning(disable: 4805)  // unsafe mix of type 'type' and type 'type' in operation
#endif

    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->assign(name, value, op);
        }
    }
    else {
        auto newValue = std::visit(
            Utils::Overload{
                [op](auto variable, auto assign)
                { 
                    if(op == Type::EQUAL) {
                        return Token::LiteralValue(assign);
                    }
                    else if(op == Type::STAR_EQUAL) {
                        return Token::LiteralValue(variable * assign);
                    }
                    else if(op == Type::SLASH_EQUAL) {
                        return Token::LiteralValue(variable / assign);
                    }
                    else if(op == Type::PLUS_EQUAL) {
                        return Token::LiteralValue(variable + assign);
                    }
                    else if(op == Type::MINUS_EQUAL) {
                        return Token::LiteralValue(variable - assign);
                    }
                    else if constexpr(std::is_integral_v<decltype(variable)> && std::is_integral_v<decltype(assign)>) {
                        if(op == Type::PERCENT_EQUAL) {
                            return Token::LiteralValue(variable % assign);
                        }
                        else if(op == Type::AMPERSAND_EQUAL) {
                            return Token::LiteralValue(variable & assign);
                        }
                        else if(op == Type::CARET_EQUAL) {
                            return Token::LiteralValue(variable ^ assign);
                        }
                        else if(op == Type::PIPE_EQUAL) {
                            return Token::LiteralValue(variable | assign);
                        }
                        else if(op == Type::SHIFT_LEFT_EQUAL) {
                            return Token::LiteralValue(variable << assign);
                        }
                        else if(op == Type::SHIFT_RIGHT_EQUAL) {
                            return Token::LiteralValue(variable >> assign);
                        }
                    }
                    throw std::runtime_error("Unsupported assignment operation");
                },
                [op](std::monostate, auto assign) 
                { 
                    if(op == Type::EQUAL) {
                        return Token::LiteralValue(assign);
                    }
                    else {
                        throw std::runtime_error("Invalid assignment operand");
                    }
                },
                [](std::monostate, std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid assignment operand"); },
                [](auto, std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid assignment operand"); }},
            std::get<Token::LiteralValue>(variable->second), value);

        // Update environemnt with new value and return
        std::get<Token::LiteralValue>(variable->second) = newValue;
        return newValue;
    }

#ifdef _WIN32
    #pragma warning(pop)
#endif
}
//---------------------------------------------------------------------------
Interpreter::Value Interpreter::Environment::prefixIncDec(const Token &name, Token::Type op)
{
    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->prefixIncDec(name, op);
        }
    }
    else {
        // Perform operation on variable value
        std::get<Token::LiteralValue>(variable->second) = std::visit(
            Utils::Overload{
                [op](auto variable)
                { 
                    if(op == Type::PLUS_PLUS) {
                        return Token::LiteralValue(variable + 1);
                    }
                    else if(op == Type::MINUS_MINUS) {
                        return Token::LiteralValue(variable - 1);
                    }
                    else {
                        throw std::runtime_error("Unsupported prefix operation");
                    }
                },
                [](std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid prefix operand"); }},
            std::get<Token::LiteralValue>(variable->second));

        // Return updated value
        return std::get<Token::LiteralValue>(variable->second);
    }
}
//---------------------------------------------------------------------------
Interpreter::Value Interpreter::Environment::postfixIncDec(const Token &name, Token::Type op)
{
    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->postfixIncDec(name, op);
        }
    }
    else {
        // Perform operation on variable value
        const auto prevValue = std::get<Token::LiteralValue>(variable->second);
        std::get<Token::LiteralValue>(variable->second) = std::visit(
            Utils::Overload{
                [op](auto variable)
                { 
                    if(op == Type::PLUS_PLUS) {
                        return Token::LiteralValue(variable + 1);
                    }
                    else if(op == Type::MINUS_MINUS) {
                        return Token::LiteralValue(variable - 1);
                    }
                    else {
                        throw std::runtime_error("Unsupported postfix operation");
                    }
                },
                [](std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid postfix operand"); }},
            std::get<Token::LiteralValue>(variable->second));

        // Return previous value
        return prevValue;
    }
}
//---------------------------------------------------------------------------
Interpreter::Value Interpreter::Environment::get(const Token &name) const
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
    return std::get<Token::LiteralValue>(m_Value);
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
    m_Value = m_Environment->assign(assignment.getVarName(), value, assignment.getOperator().type);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Binary &binary)
{
#ifdef _WIN32
    #pragma warning(push)
    #pragma warning(disable: 4804)  // unsafe use of type 'bool' in operation
    #pragma warning(disable: 4805)  // unsafe mix of type 'type' and type 'type' in operation
    #pragma warning(disable: 4018)  // signed/unsigned mismatch
#endif
    using Type = Token::Type;

    auto leftValue = evaluate(binary.getLeft());
    auto rightValue = evaluate(binary.getRight());

    const auto opType = binary.getOperator().type;
    m_Value = std::visit(
        Utils::Overload{
            [opType](auto left, auto right)->Token::LiteralValue
            {
                if(opType == Type::COMMA) {
                    return Token::LiteralValue(right);
                }
                else if(opType == Type::PLUS) {
                    return Token::LiteralValue(left + right);
                }
                else if(opType == Type::MINUS) {
                    return Token::LiteralValue(left - right);
                }
                else if(opType == Type::STAR) {
                    return Token::LiteralValue(left * right);
                }
                else if(opType == Type::SLASH) {
                    return Token::LiteralValue(left / right);
                }
                else if(opType == Type::GREATER) {
                    return Token::LiteralValue(left > right);
                }
                else if(opType == Type::GREATER_EQUAL) {
                    return Token::LiteralValue(left >= right);
                }
                else if(opType == Type::LESS) {
                    return Token::LiteralValue(left < right);
                }
                else if(opType == Type::LESS_EQUAL) {
                    return Token::LiteralValue(left < right);
                }
                else if(opType == Type::NOT_EQUAL) {
                    return Token::LiteralValue(left != right);
                }
                else if(opType == Type::EQUAL_EQUAL) {
                    return Token::LiteralValue(left == right);
                }
                else if constexpr(std::is_integral_v<decltype(left)> && std::is_integral_v<decltype(right)>) {
                    if(opType == Type::PERCENT) {
                        return left % right;
                    }
                    else if(opType == Type::SHIFT_LEFT) {
                        return left << right;
                    }
                    else if(opType == Type::SHIFT_RIGHT) {
                        return left >> right;
                    }
                    else if(opType == Type::CARET) {
                        return Token::LiteralValue(left ^ right);
                    }
                    else if(opType == Type::AMPERSAND) {
                        return Token::LiteralValue(left & right);
                    }
                    else if(opType == Type::PIPE) {
                        return Token::LiteralValue(left | right);
                    }
                }
                throw std::runtime_error("Unsupported binary operation");
            },
            [](std::monostate, std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid operand"); },
            [](std::monostate, auto)->Token::LiteralValue { throw std::runtime_error("Invalid operand"); },
            [](auto, std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid operand"); }},
        leftValue, rightValue);
#ifdef _WIN32
    #pragma warning(pop)
#endif
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Call &call)
{
    // Evaluate callee and extract callabale
    // **NOTE** we can't call evaluate as that returns a value
    call.getCallee()->accept(*this);
    auto callable = std::get<std::reference_wrapper<Callable>>(m_Value);

    // Evaluate arguments
    std::vector<Token::LiteralValue> arguments;
    for(const auto &arg : call.getArguments()) {
        arguments.push_back(evaluate(arg.get()));
    }

    // If arguments count doesn't match, give error
    if(arguments.size() != callable.get().getArity()) {
        throw std::runtime_error("Expected " + std::to_string(callable.get().getArity()) + " arguments but got "
                                 + std::to_string(arguments.size()) + " at line:" + std::to_string(call.getClosingParen().line));
    }

    // Call function and save result
    m_Value = callable.get().call(arguments);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Cast &cast)
{
    m_Value = evaluate(cast.getExpression());

    assert(false);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Conditional &conditional)
{
    auto conditionValue = evaluate(conditional.getCondition());

    if(isTruthy(conditionValue)) {
        m_Value = evaluate(conditional.getTrue());
    }
    else {
        m_Value = evaluate(conditional.getFalse());
    }
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
        }
        else {
            m_Value = (int)isTruthy(evaluate(logical.getRight()));
        }
    }
    else {
        if(!isTruthy(leftValue)) {
            m_Value = 0;
        }
        else {
            m_Value = (int)isTruthy(evaluate(logical.getRight()));
        }
    }
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::PostfixIncDec &postfixIncDec)
{
    m_Value = m_Environment->postfixIncDec(postfixIncDec.getVarName(), postfixIncDec.getOperator().type);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::PrefixIncDec &prefixIncDec)
{
    m_Value = m_Environment->prefixIncDec(prefixIncDec.getVarName(), prefixIncDec.getOperator().type);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Variable &variable)
{
    m_Value = m_Environment->get(variable.getName());
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Expression::Unary &unary)
{
#ifdef _WIN32
    #pragma warning(push)
    #pragma warning(disable: 4146)  // unary minus operator applied to unsigned type, result still unsigned
    #pragma warning(disable: 4804)  // '~': unsafe use of type 'bool' in operation
#endif
    using Type = Token::Type;

    auto rightValue = evaluate(unary.getRight());

    const auto opType = unary.getOperator().type;
    m_Value = std::visit(
        Utils::Overload{
            [opType](auto right)->Token::LiteralValue
            {
                if(opType == Type::PLUS) {
                    return Token::LiteralValue(+right);
                }
                else if(opType == Type::MINUS) {
                    return Token::LiteralValue(-right);
                }
                else if(opType == Type::NOT) {
                    return Token::LiteralValue(!right);
                }
                else if constexpr(std::is_integral_v<decltype(right)>) {
                    if(opType == Type::TILDA) {
                        return Token::LiteralValue(~right);
                    }
                }

                throw std::runtime_error("Unsupported unary operation");
            },
            [](std::monostate)->Token::LiteralValue { throw std::runtime_error("Invalid operand"); }},
        rightValue);
#ifdef _WIN32
    #pragma warning(pop)
#endif
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Compound &compound)
{
    Environment environment(m_Environment);
    interpret(compound.getStatements(), environment);
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Do &doStatement)
{
    do {
        doStatement.getBody()->accept(*this);
    } while(isTruthy(evaluate(doStatement.getCondition())));
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::Expression &expression)
{
    evaluate(expression.getExpression());
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::For &forStatement)
{
    // Create new environment for loop initialisation
    Environment *previous = m_Environment;
    Environment environment(m_Environment);
    m_Environment = &environment;

    // Interpret initialiser if statement present
    if(forStatement.getInitialiser()) {
        forStatement.getInitialiser()->accept(*this);
    }

    // While condition is true
    while(isTruthy(evaluate(forStatement.getCondition()))) {
        // Interpret body
        forStatement.getBody()->accept(*this);

        // Interpret incrementer if present
        if(forStatement.getIncrement()) {
            forStatement.getIncrement()->accept(*this);
        }
    }

    // Restore environment
    m_Environment = previous;
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
            value = std::get<Token::LiteralValue>(m_Value);
        }
        m_Environment->define(std::get<0>(var), value);
    }
}
//---------------------------------------------------------------------------
void Interpreter::visit(const Statement::While &whileStatement)
{
    while(isTruthy(evaluate(whileStatement.getCondition()))) {
        whileStatement.getBody()->accept(*this);
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