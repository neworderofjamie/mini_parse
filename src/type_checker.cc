#include "type_checker.h"

// Standard C++ includes
#include <string>

// Standard C includes
#include <cassert>

// GeNN includes
#include "type.h"

// Mini-parse includes
#include "utils.h"

//---------------------------------------------------------------------------
// MiniParse::TypeChecker::Environment
//---------------------------------------------------------------------------
namespace MiniParse
{
void TypeChecker::Environment::define(const Token &name, const Type::Base *type, bool isConst)
{
    if(!m_Types.try_emplace(name.lexeme, type, isConst).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
}
//---------------------------------------------------------------------------
const Type::Base *TypeChecker::Environment::assign(const Token &name, const Type::Base *type)
{
    auto existingType = m_Types.find(name.lexeme);
    if(existingType == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->assign(name, type);
        }
    }
    // Otherwise, if type is found and it's const, give error
    else if(std::get<1>(existingType->second)) {
        throw std::runtime_error("assignment of read-only variable '" + std::string{name.lexeme} + "'");
    }
    // Otherwise, return type
    else {
        return std::get<0>(existingType->second);
    }
}
//---------------------------------------------------------------------------
const Type::Base *TypeChecker::Environment::incDec(const Token &name)
{
    auto existingType = m_Types.find(name.lexeme);
    if(existingType == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->incDec(name);
        }
    }
    // Otherwise, if type is found and it's const, give error
    else if(std::get<1>(existingType->second)) {
        throw std::runtime_error("increment/decrement of read-only variable '" + std::string{name.lexeme} + "'");
    }
    // Otherwise, return type
    else {
        return std::get<0>(existingType->second);
    }
}
//---------------------------------------------------------------------------
std::tuple<const Type::Base*, bool> TypeChecker::Environment::getType(const Token &name) const
{
    auto type = m_Types.find(std::string{name.lexeme});
    if(type == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            return m_Enclosing->getType(name);
        }
    }
    else {
        return type->second;
    }
}

//---------------------------------------------------------------------------
// MiniParse::TypeChecker
//---------------------------------------------------------------------------
void TypeChecker::typeCheck(const Statement::StatementList &statements, Environment &environment)
{
    Environment *previous = m_Environment;
    m_Environment = &environment;
    for(auto &s : statements) {
        s.get()->accept(*this);
    }
    m_Environment = previous;
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Assignment &assignment)
{
    auto rhsType = evaluateType(assignment.getValue());
    m_Type = m_Environment->assign(assignment.getVarName(), rhsType);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Binary &binary)
{
    const auto opType = binary.getOperator().type;
    if(opType == Token::Type::COMMA) {
        m_Type = evaluateType(binary.getRight());
    }
    else {
        auto leftType = evaluateType(binary.getLeft());
        auto rightType = evaluateType(binary.getRight());
        auto leftNumericType = dynamic_cast<const Type::NumericBase *>(leftType);
        auto rightNumericType = dynamic_cast<const Type::NumericBase *>(rightType);
        if(leftNumericType == nullptr || rightNumericType == nullptr) {
            throw std::runtime_error("Invalid operand types '" + leftType->getTypeName() + "' and '" + rightType->getTypeName() + "' to binary " + std::string{binary.getOperator().lexeme});
        }
        else if(opType == Token::Type::SHIFT_LEFT || opType == Token::Type::SHIFT_RIGHT) {
            m_Type = Type::getPromotedType(leftNumericType);
        }
        else {
            m_Type = Type::getCommonType(leftNumericType, rightNumericType);
        }
    }
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Call &call)
{
    // Evaluate callee type
    // **NOTE** we can't call evaluate as that returns a value
    auto calleeType = evaluateType(call.getCallee());
    auto calleeFunctionType = dynamic_cast<const Type::ForeignFunctionBase *>(calleeType);

    // If callee isn't a function at all, give error
    if(calleeFunctionType == nullptr) {
        throw std::runtime_error("Called object is not a function");
    }
    // Otherwise
    else {
        // If argument count doesn't match
        const auto argTypes = calleeFunctionType->getArgumentTypes();
        if(call.getArguments().size() < argTypes.size()) {
            throw std::runtime_error("Too many arguments to function");
        }
        else if(call.getArguments().size() > argTypes.size()) {
            throw std::runtime_error("Too few arguments to function");
        }
        else {
            // Loop through arguments
            // **TODO** check
            /*for(size_t i = 0; i < argTypes.size(); i++) {
                // Evaluate argument type 
                auto callArgType = evaluateType(call.getArguments().at(i).get());
            }*/
            // Type is return type of function
            m_Type = calleeFunctionType->getReturnType();
        }
    }
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Cast &cast)
{
    m_Type = cast.getType();
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Conditional &conditional)
{
    auto trueType = evaluateType(conditional.getTrue());
    auto falseType = evaluateType(conditional.getFalse());
    auto trueNumericType = dynamic_cast<const Type::NumericBase*>(trueType);
    auto falseNumericType = dynamic_cast<const Type::NumericBase*>(falseType);
    if(trueNumericType == nullptr || falseNumericType == nullptr) {
        throw std::runtime_error("Invalid operand types '" + trueType->getTypeName() + "' and '" + std::string{falseType->getTypeName()} + "' to conditional");
    }
    else {
        m_Type = Type::getCommonType(trueNumericType, falseNumericType);
    }
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Grouping &grouping)
{
    m_Type = evaluateType(grouping.getExpression());
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Literal &literal)
{
    m_Type = std::visit(
        MiniParse::Utils::Overload{
            [](auto v)->const Type::NumericBase*{ return Type::TypeTraits<decltype(v)>::NumericType::getInstance(); },
            [](std::monostate)->const Type::NumericBase* { return nullptr; }},
        literal.getValue());
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Logical &logical)
{
    logical.getLeft()->accept(*this);
    logical.getRight()->accept(*this);
    m_Type = Type::Int32::getInstance();
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::PostfixIncDec &postfixIncDec)
{
    m_Type = m_Environment->incDec(postfixIncDec.getVarName());
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::PrefixIncDec &prefixIncDec)
{
    m_Type = m_Environment->incDec(prefixIncDec.getVarName());
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Variable &variable)
{
    m_Type = std::get<0>(m_Environment->getType(variable.getName()));
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Unary &unary)
{
    auto rightType = evaluateType(unary.getRight());
    auto rightNumericType = dynamic_cast<const Type::NumericBase*>(rightType);
    if(rightNumericType == nullptr) {
        throw std::runtime_error("Invalid operand type '" + rightType->getTypeName() + "' to unary " + std::string{unary.getOperator().lexeme});
    }
    else {
        // If operator is arithmetic, return promoted type
        if(unary.getOperator().type == Token::Type::PLUS || unary.getOperator().type == Token::Type::MINUS) {
            m_Type = Type::getPromotedType(rightNumericType);
        }
        // Otherwise, if operator is bitwise
        else if(unary.getOperator().type == Token::Type::TILDA) {
            // Check type is integer
            if(!rightNumericType->isIntegral()) {
                throw std::runtime_error("Bitwise complement operator does not support type '" + rightNumericType->getTypeName() + "'");
            }

            // Return promoted type
            m_Type = Type::getPromotedType(rightNumericType);
        }
        // Otherwise, if operator is logical
        else if(unary.getOperator().type == Token::Type::NOT) {
            m_Type = Type::Int32::getInstance();
        }
    }
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::Compound &compound)
{
    Environment environment(m_Environment);
    typeCheck(compound.getStatements(), environment);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::Do &doStatement)
{
    doStatement.getBody()->accept(*this);
    doStatement.getCondition()->accept(*this);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::Expression &expression)
{
    expression.getExpression()->accept(*this);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::For &forStatement)
{
    // Create new environment for loop initialisation
    Environment *previous = m_Environment;
    Environment environment(m_Environment);
    m_Environment = &environment;

    // Interpret initialiser if statement present
    if(forStatement.getInitialiser()) {
        forStatement.getInitialiser()->accept(*this);
    }
    
    if(forStatement.getCondition()) {
        forStatement.getCondition()->accept(*this);
    }

    if(forStatement.getIncrement()) {
        forStatement.getIncrement()->accept(*this);
    }

    forStatement.getBody()->accept(*this);

    // Restore environment
    m_Environment = previous;
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::If &ifStatement)
{
    ifStatement.getCondition()->accept(*this);
    ifStatement.getThenBranch()->accept(*this);
    ifStatement.getElseBranch()->accept(*this);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::VarDeclaration &varDeclaration)
{
    for(const auto &var : varDeclaration.getInitDeclaratorList()) {
        m_Environment->define(std::get<0>(var), varDeclaration.getType(), varDeclaration.isConst());
    }
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::While &whileStatement)
{
    whileStatement.getCondition()->accept(*this);
    whileStatement.getBody()->accept(*this);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::Print &print)
{
    print.getExpression()->accept(*this);
}
//---------------------------------------------------------------------------
const Type::Base* TypeChecker::evaluateType(const Expression::Base *expression)
{
    expression->accept(*this);
    return m_Type;
}
}   // namespace MiniParse