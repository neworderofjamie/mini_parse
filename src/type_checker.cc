#include "type_checker.h"

// Standard C++ includes
#include <stdexcept>
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
void TypeChecker::Environment::define(const Token &name, const Type::NumericBase *type, bool isConst)
{
    if(!m_Types.try_emplace(name.lexeme, type, isConst).second) {
        throw std::runtime_error("Redeclaration of '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
    }
}
//---------------------------------------------------------------------------
void TypeChecker::Environment::assign(const Token &name, const Type::NumericBase *type)
{
    auto existingType = m_Types.find(name.lexeme);
    if(existingType == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            throw std::runtime_error("Undefined variable '" + std::string{name.lexeme} + "' at line " + std::to_string(name.line));
        }
        else {
            m_Enclosing->assign(name, type);
        }
    }
    // Otherwise, if type is found and it's const, give error
    else if(std::get<1>(existingType->second)) {
        throw std::runtime_error("assignment of read-only variable '" + std::string{name.lexeme} + "'");
    }
}
//---------------------------------------------------------------------------
std::tuple<const Type::NumericBase*, bool> TypeChecker::Environment::getType(const Token &name) const
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
    m_Environment->assign(assignment.getVarName(), rhsType);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Binary &binary)
{
    auto leftType = evaluateType(binary.getLeft());
    auto rightType = evaluateType(binary.getRight());
    m_Type = Type::getCommonType(leftType, rightType);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Call &call)
{
    assert(false);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Conditional &conditional)
{
    auto trueType = evaluateType(conditional.getTrue());
    auto falseType = evaluateType(conditional.getFalse());
    m_Type = Type::getCommonType(trueType, falseType);
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
void TypeChecker::visit(const Expression::Variable &variable)
{
    m_Type = std::get<0>(m_Environment->getType(variable.getName()));
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Expression::Unary &unary)
{
    auto rightType = evaluateType(unary.getRight());

    // If operator is arithmetic, return promoted type
    if(unary.getOperator().type == Token::Type::PLUS || unary.getOperator().type == Token::Type::MINUS) {
        m_Type = Type::getPromotedType(rightType);
    }
    // Otherwise, if operator is bitwise
    else if(unary.getOperator().type == Token::Type::TILDA) {
        // Check type is integer
        if(!rightType->isIntegral()) {
            throw std::runtime_error("Bitwise complement operator does not support type '" + std::string{rightType->getTypeName()} + "'");
        }

        // Return promoted type
        m_Type = Type::getPromotedType(rightType);
    }
    // Otherwise, if operator is logical
    else if(unary.getOperator().type == Token::Type::NOT) {
        m_Type = Type::Int32::getInstance();
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

    // Restore environment
    m_Environment = previous;
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::If &ifStatement)
{
    ifStatement.getThenBranch()->accept(*this);
    ifStatement.getElseBranch()->accept(*this);
}
//---------------------------------------------------------------------------
void TypeChecker::visit(const Statement::VarDeclaration &varDeclaration)
{
    // **TODO** something with type
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
const Type::NumericBase* TypeChecker::evaluateType(const Expression::Base *expression)
{
    expression->accept(*this);
    return m_Type;
}
}   // namespace MiniParse