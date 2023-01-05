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

using namespace MiniParse;
using namespace MiniParse::Interpreter;

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

//---------------------------------------------------------------------------
// Break
//---------------------------------------------------------------------------
class Break
{
};

//---------------------------------------------------------------------------
// Continue
//---------------------------------------------------------------------------
class Continue
{
};

//---------------------------------------------------------------------------
// Visitor
//---------------------------------------------------------------------------
class Visitor : public Expression::Visitor, public Statement::Visitor
{
public:
    Visitor()
    :   m_Environment(nullptr)
    {
    }
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    Token::LiteralValue evaluate(const Expression::Base *expression)
    {
        expression->accept(*this);
        return std::get<Token::LiteralValue>(m_Value);
    }

    void interpret(const Statement::StatementList &statements, Environment &environment)
    {
        Environment *previous = m_Environment;
        m_Environment = &environment;
        for(auto &s : statements) {
            s.get()->accept(*this);
        }
        m_Environment = previous;
    }

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::ArraySubscript &arraySubscript) final
    {
        assert(false);
    }

    virtual void visit(const Expression::Assignment &assignment) final
    {
        auto value = evaluate(assignment.getValue());
        m_Value = m_Environment->assign(assignment.getVarName(), value, assignment.getOperator().type);
    }

    virtual void visit(const Expression::Binary &binary) final
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

    virtual void visit(const Expression::Call &call) final
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

        // If callable has fixed arity
        if(callable.get().getArity()) {
            //If arguments count doesn't match, give error
            const size_t callableArity = *callable.get().getArity();
            if(arguments.size() != callableArity) {
                throw std::runtime_error("Expected " + std::to_string(callableArity) + " arguments but got "
                                         + std::to_string(arguments.size()) + " at line:" + std::to_string(call.getClosingParen().line));
            }
        }

        // Call function and save result
        m_Value = callable.get().call(arguments);
    }

    virtual void visit(const Expression::Cast &cast) final
    {
        m_Value = evaluate(cast.getExpression());

        assert(false);
    }

    virtual void visit(const Expression::Conditional &conditional) final
    {
        auto conditionValue = evaluate(conditional.getCondition());

        if(isTruthy(conditionValue)) {
            m_Value = evaluate(conditional.getTrue());
        }
        else {
            m_Value = evaluate(conditional.getFalse());
        }
    }

    virtual void visit(const Expression::Grouping &grouping) final
    {
        evaluate(grouping.getExpression());
    }

    virtual void visit(const Expression::Literal &literal) final
    {
        m_Value = literal.getValue();
    }

    virtual void visit(const Expression::Logical &logical) final
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

    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) final
    {
        m_Value = m_Environment->postfixIncDec(postfixIncDec.getVarName(), postfixIncDec.getOperator().type);
    }

    virtual void visit(const Expression::PrefixIncDec &prefixIncDec) final
    {
        m_Value = m_Environment->prefixIncDec(prefixIncDec.getVarName(), prefixIncDec.getOperator().type);
    }

    virtual void visit(const Expression::Variable &variable) final
    {
        m_Value = m_Environment->get(variable.getName());
    }

    virtual void visit(const Expression::Unary &unary) final
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
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Break &breakStatement) final
    {
        throw Break();
    }

    virtual void visit(const Statement::Compound &compound) final
    {
        Environment environment(m_Environment);
        interpret(compound.getStatements(), environment);
    }

    virtual void visit(const Statement::Continue &continueStatement) final
    {
         throw Continue();
    }

    virtual void visit(const Statement::Do &doStatement) final
    {
        do {
            try {
                doStatement.getBody()->accept(*this);
            }
            // Break if we encounter break exception and ignore continue exceptions
            catch(Break&) {
                break;
            }
            catch(Continue&) {
            }
        
        } while(isTruthy(evaluate(doStatement.getCondition())));
    }

    virtual void visit(const Statement::Expression &expression) final
    {
        evaluate(expression.getExpression());
    }

    virtual void visit(const Statement::For &forStatement) final
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
            try {
                forStatement.getBody()->accept(*this);
            }
            // Break if we encounter break exception and ignore continue exceptions
            catch(Break&) {
                break;
            }
            catch(Continue&) {
            }

            // Interpret incrementer if present
            if(forStatement.getIncrement()) {
                forStatement.getIncrement()->accept(*this);
            }
        }

        // Restore environment
        m_Environment = previous;
    }

    virtual void visit(const Statement::If &ifStatement) final
    {
        if(isTruthy(evaluate(ifStatement.getCondition()))) {
            ifStatement.getThenBranch()->accept(*this);
        }
        else if(ifStatement.getElseBranch()) {
            ifStatement.getElseBranch()->accept(*this);
        }
    }

    virtual void visit(const Statement::Labelled &labelled) final
    {
        // If label has a value i.e. it's a case not a default, evaluate value and add to vector of labelled statements
        if (labelled.getValue()) {
            m_SwitchLabelledStatements.emplace_back(evaluate(labelled.getValue()), 
                                                    labelled.getBody());
        }
        // Otherwise, add body to vector of labelled statements
        else {
            m_SwitchLabelledStatements.emplace_back(std::monostate(), labelled.getBody());
        }
    }

    virtual void visit(const Statement::Switch &switchStatement) final
    {
        // **TODO** handle nested switch statements
        assert(m_SwitchLabelledStatements.empty());

        // Visit switch statement body to find labelled statements
        switchStatement.getBody()->accept(*this);

        // Evaluate value
        auto value = evaluate(switchStatement.getCondition());

        bool matched = false;
        for (const auto &s : m_SwitchLabelledStatements) {
            try {
                // If we've already matched out value or this case matches it 
                if (matched || value == s.first) {
                    s.second->accept(*this);
                    matched = true;
                }
            }
            // Break if we encounter break exception and ignore continue exceptions
            catch(Break&) {
                break;
            }
        }
        
        // If a case hasn't yet been found, look for default
        if (!matched) {
            for (const auto &s : m_SwitchLabelledStatements) {
                try {
                    // If we've already matched our value or this is a default
                    if (matched || std::holds_alternative<std::monostate>(s.first)) {
                        s.second->accept(*this);
                        matched = true;
                    }
                }
                // Break if we encounter break exception and ignore continue exceptions
                catch(Break&) {
                    break;
                }
            }
            
        }

        // Clear out labelled statements
        // **THINK** exception safety
        m_SwitchLabelledStatements.clear();
    }

    virtual void visit(const Statement::VarDeclaration &varDeclaration) final
    {
        // **TODO** something with type
        for(const auto &var : varDeclaration.getInitDeclaratorList()) {
            Token::LiteralValue value;
            if(std::get<1>(var)) {
                evaluate(std::get<1>(var).get());
                value = std::get<Token::LiteralValue>(m_Value);
            }
            m_Environment->define(std::get<0>(var), value);
        }
    }

    virtual void visit(const Statement::While &whileStatement) final
    {
        while(isTruthy(evaluate(whileStatement.getCondition()))) {
            try {
                whileStatement.getBody()->accept(*this);
            }
            // Break if we encounter break exception and ignore continue exceptions
            catch(Break&) {
                break;
            }
            catch(Continue&) {
            }
        }
    }

    virtual void visit(const Statement::Print &print) final
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

private:
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Environment::Value m_Value;
    std::vector<std::pair<Token::LiteralValue, const Statement::Base*>> m_SwitchLabelledStatements;
    
    Environment *m_Environment;
};
}

//---------------------------------------------------------------------------
// MiniParse::Interpreter::Environment
//---------------------------------------------------------------------------
namespace MiniParse::Interpreter
{
void Environment::define(const Token &name, Token::LiteralValue value)
{
    if(!m_Values.try_emplace(name.lexeme, value).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
}
//---------------------------------------------------------------------------
void Environment::define(std::string_view name, Callable &callable)
{
    if(!m_Values.try_emplace(name, callable).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name});
    }
}
//---------------------------------------------------------------------------
Environment::Value Environment::assign(const Token &name, Token::LiteralValue value, Token::Type op)
{
#ifdef _WIN32
    #pragma warning(push)
    #pragma warning(disable: 4804)  // unsafe use of type 'bool' in operation
    #pragma warning(disable: 4805)  // unsafe mix of type 'type' and type 'type' in operation
#endif

    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing) {
            return m_Enclosing->assign(name, value, op);
        }
        else {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
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
Environment::Value Environment::prefixIncDec(const Token &name, Token::Type op)
{
    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing) {
            return m_Enclosing->prefixIncDec(name, op);
        }
        else {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
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
Environment::Value Environment::postfixIncDec(const Token &name, Token::Type op)
{
    using Type = Token::Type;

    auto variable = m_Values.find(name.lexeme);
    if(variable == m_Values.end()) {
        if(m_Enclosing) {
            return m_Enclosing->postfixIncDec(name, op);
        }
        else {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
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
Environment::Value Environment::get(const Token &name) const
{
    auto val = m_Values.find(std::string{name.lexeme});
    if(val == m_Values.end()) {
        if(m_Enclosing) {
            return m_Enclosing->get(name);
        }
        else {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
    }
    else {
        return val->second;
    }
}

void interpret(const Statement::StatementList &statements, Environment &environment)
{
    Visitor interpreter;
    interpreter.interpret(statements, environment);
}
}   // namespace MiniParse::Interpreter