#include "type_checker.h"

// Standard C++ includes
#include <string>

// Standard C includes
#include <cassert>

// GeNN includes
#include "type.h"

// Mini-parse includes
#include "error_handler.h"
#include "expression.h"
#include "utils.h"

using namespace MiniParse;
using namespace MiniParse::TypeChecker;

//---------------------------------------------------------------------------
// Anonymous namespace
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// TypeCheckError
//---------------------------------------------------------------------------
class TypeCheckError
{
};

//---------------------------------------------------------------------------
// Vistor
//---------------------------------------------------------------------------
class Visitor : public Expression::Visitor, public Statement::Visitor
{
public:
    Visitor(ErrorHandler &errorHandler)
        : m_Environment(nullptr), m_Type(nullptr), m_ErrorHandler(errorHandler)
    {
    }
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    void typeCheck(const Statement::StatementList &statements, Environment &environment)
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
    virtual void visit(const Expression::Assignment &assignment) final
    {
        auto rhsType = evaluateType(assignment.getValue());
        m_Type = m_Environment->assign(assignment.getVarName(), rhsType, m_ErrorHandler);
    }

    virtual void visit(const Expression::Binary &binary) final
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
                m_ErrorHandler.error(binary.getOperator(), "Invalid operand types '" + leftType->getTypeName() + "' and '" + rightType->getTypeName());
                throw TypeCheckError();
            }
            else if(opType == Token::Type::SHIFT_LEFT || opType == Token::Type::SHIFT_RIGHT) {
                m_Type = Type::getPromotedType(leftNumericType);
            }
            else {
                m_Type = Type::getCommonType(leftNumericType, rightNumericType);
            }
        }
    }

    virtual void visit(const Expression::Call &call) final
    {
        // Evaluate callee type
        // **NOTE** we can't call evaluate as that returns a value
        auto calleeType = evaluateType(call.getCallee());
        auto calleeFunctionType = dynamic_cast<const Type::ForeignFunctionBase *>(calleeType);

        // If callee isn't a function at all, give error
        if(calleeFunctionType == nullptr) {
            m_ErrorHandler.error(call.getClosingParen(), "Called object is not a function");
            throw TypeCheckError();
        }
        // Otherwise
        else {
            // If argument count doesn't match
            const auto argTypes = calleeFunctionType->getArgumentTypes();
            if(call.getArguments().size() < argTypes.size()) {
                m_ErrorHandler.error(call.getClosingParen(), "Too many arguments to function");
                throw TypeCheckError();
            }
            else if(call.getArguments().size() > argTypes.size()) {
                m_ErrorHandler.error(call.getClosingParen(), "Too few arguments to function");
                throw TypeCheckError();
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

    virtual void visit(const Expression::Cast &cast) final
    {
        m_Type = cast.getType();
    }

    virtual void visit(const Expression::Conditional &conditional) final
    {
        auto trueType = evaluateType(conditional.getTrue());
        auto falseType = evaluateType(conditional.getFalse());
        auto trueNumericType = dynamic_cast<const Type::NumericBase*>(trueType);
        auto falseNumericType = dynamic_cast<const Type::NumericBase*>(falseType);
        if(trueNumericType == nullptr || falseNumericType == nullptr) {
            m_ErrorHandler.error(conditional.getQuestion(),
                                 "Invalid operand types '" + trueType->getTypeName() + "' and '" + std::string{falseType->getTypeName()} + "' to conditional");
            throw TypeCheckError();
        }
        else {
            m_Type = Type::getCommonType(trueNumericType, falseNumericType);
        }
    }

    virtual void visit(const Expression::Grouping &grouping) final
    {
        m_Type = evaluateType(grouping.getExpression());
    }

    virtual void visit(const Expression::Literal &literal) final
    {
        m_Type = std::visit(
            MiniParse::Utils::Overload{
                [](auto v)->const Type::NumericBase*{ return Type::TypeTraits<decltype(v)>::NumericType::getInstance(); },
                [](std::monostate)->const Type::NumericBase* { return nullptr; }},
            literal.getValue());
    }

    virtual void visit(const Expression::Logical &logical) final
    {
        logical.getLeft()->accept(*this);
        logical.getRight()->accept(*this);
        m_Type = Type::Int32::getInstance();
    }

    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) final
    {
        m_Type = m_Environment->incDec(postfixIncDec.getVarName(), m_ErrorHandler);
    }

    virtual void visit(const Expression::PrefixIncDec &prefixIncDec) final
    {
        m_Type = m_Environment->incDec(prefixIncDec.getVarName(), m_ErrorHandler);
    }

    virtual void visit(const Expression::Variable &variable)
    {
        m_Type = std::get<0>(m_Environment->getType(variable.getName(), m_ErrorHandler));
    }

    virtual void visit(const Expression::Unary &unary) final
    {
        auto rightType = evaluateType(unary.getRight());
        auto rightNumericType = dynamic_cast<const Type::NumericBase*>(rightType);
        if(rightNumericType == nullptr) {
            m_ErrorHandler.error(unary.getOperator(), 
                                 "Invalid operand type '" + rightType->getTypeName() + "'");
            throw TypeCheckError();
        }
        else {
            // If operator is arithmetic, return promoted type
            if(unary.getOperator().type == Token::Type::PLUS || unary.getOperator().type == Token::Type::MINUS) {
                m_Type = Type::getPromotedType(rightNumericType);
            }
            // Otherwise, if operator is bitwise
            else if(unary.getOperator().type == Token::Type::TILDA) {
                // If type is integer, return promoted type
                if(rightNumericType->isIntegral()) {
                    m_Type = Type::getPromotedType(rightNumericType);
                }
                else {
                    m_ErrorHandler.error(unary.getOperator(),
                                         "Invalid operand type '" + rightType->getTypeName() + "'");
                    throw TypeCheckError();
                }
            }
            // Otherwise, if operator is logical
            else if(unary.getOperator().type == Token::Type::NOT) {
                m_Type = Type::Int32::getInstance();
            }
        }
    }

    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Break &) final
    {
    }

    virtual void visit(const Statement::Compound &compound) final
    {
        Environment environment(m_Environment);
        typeCheck(compound.getStatements(), environment);
    }

    virtual void visit(const Statement::Continue &) final
    {
    }

    virtual void visit(const Statement::Do &doStatement) final
    {
        doStatement.getBody()->accept(*this);
        doStatement.getCondition()->accept(*this);
    }

    virtual void visit(const Statement::Expression &expression) final
    {
        expression.getExpression()->accept(*this);
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

    virtual void visit(const Statement::If &ifStatement) final
    {
        ifStatement.getCondition()->accept(*this);
        ifStatement.getThenBranch()->accept(*this);
        if(ifStatement.getElseBranch() != nullptr) {
            ifStatement.getElseBranch()->accept(*this);
        }
    }

    virtual void visit(const Statement::VarDeclaration &varDeclaration) final
    {
        for(const auto &var : varDeclaration.getInitDeclaratorList()) {
            m_Environment->define(std::get<0>(var), varDeclaration.getType(), 
                                  varDeclaration.isConst(), m_ErrorHandler);
        }
    }

    virtual void visit(const Statement::While &whileStatement) final
    {
        whileStatement.getCondition()->accept(*this);
        whileStatement.getBody()->accept(*this);
    }

    virtual void visit(const Statement::Print &print) final
    {
        print.getExpression()->accept(*this);
    }

private:
    //---------------------------------------------------------------------------
    // Private methods
    //---------------------------------------------------------------------------
    const Type::Base *evaluateType(const Expression::Base *expression)
    {
        expression->accept(*this);
        return m_Type;
    }

    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Environment *m_Environment;
    const Type::Base *m_Type;
    ErrorHandler &m_ErrorHandler;
};
}

//---------------------------------------------------------------------------
// MiniParse::TypeChecker::Environment
//---------------------------------------------------------------------------
void Environment::define(const Token &name, const Type::Base *type, bool isConst, ErrorHandler &errorHandler)
{
    if(!m_Types.try_emplace(name.lexeme, type, isConst).second) {
        errorHandler.error(name, "Redeclaration of variable");
        throw TypeCheckError();
    }
}
//---------------------------------------------------------------------------
const Type::Base *Environment::assign(const Token &name, const Type::Base *type, ErrorHandler &errorHandler)
{
    auto existingType = m_Types.find(name.lexeme);
    if(existingType == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            errorHandler.error(name, "Undefined variable");
            throw TypeCheckError();
        }
        else {
            return m_Enclosing->assign(name, type, errorHandler);
        }
    }
    // Otherwise, if type is found and it's const, give error
    else if(std::get<1>(existingType->second)) {
        errorHandler.error(name, "Assignment of read-only variable");
        throw TypeCheckError();
    }
    // Otherwise, return type
    else {
        return std::get<0>(existingType->second);
    }
}
//---------------------------------------------------------------------------
const Type::Base *Environment::incDec(const Token &name, ErrorHandler &errorHandler)
{
    auto existingType = m_Types.find(name.lexeme);
    if(existingType == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            errorHandler.error(name, "Undefined variable");
            throw TypeCheckError();
        }
        else {
            return m_Enclosing->incDec(name, errorHandler);
        }
    }
    // Otherwise, if type is found and it's const, give error
    else if(std::get<1>(existingType->second)) {
        errorHandler.error(name, "Increment/decrement of read-only variable");
        throw TypeCheckError();
    }
    // Otherwise, return type
    else {
        return std::get<0>(existingType->second);
    }
}
//---------------------------------------------------------------------------
std::tuple<const Type::Base *, bool> Environment::getType(const Token &name, ErrorHandler &errorHandler) const
{
    auto type = m_Types.find(std::string{name.lexeme});
    if(type == m_Types.end()) {
        if(m_Enclosing == nullptr) {
            errorHandler.error(name, "Undefined variable");
            throw TypeCheckError();
        }
        else {
            return m_Enclosing->getType(name, errorHandler);
        }
    }
    else {
        return type->second;
    }
}


void MiniParse::TypeChecker::typeCheck(const Statement::StatementList &statements, Environment &environment, 
                                       ErrorHandler &errorHandler)
{
    Visitor visitor(errorHandler);
    visitor.typeCheck(statements, environment);
}