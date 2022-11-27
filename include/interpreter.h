#pragma once

// Standard C++ includes
#include <optional>
#include <unordered_map>
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::Interpreter
//---------------------------------------------------------------------------
namespace MiniParse
{
class Interpreter : public Expression::Visitor, public Statement::Visitor
{
public:
    //---------------------------------------------------------------------------
    // MiniParse::Interpreter::Callable
    //---------------------------------------------------------------------------
    class Callable
    {
    public:
        virtual std::optional<size_t> getArity() const = 0;
        virtual Token::LiteralValue call(const std::vector<Token::LiteralValue> &arguments) = 0;
    };

    typedef std::variant<Token::LiteralValue, std::reference_wrapper<Callable>> Value;

    //---------------------------------------------------------------------------
    // MiniParse::Interpreter::Environment
    //---------------------------------------------------------------------------
    class Environment
    {
    public:
        Environment(Environment *enclosing = nullptr)
        :   m_Enclosing(enclosing)
        {
        }

        // **TODO** type
        void define(const Token &name, Token::LiteralValue value);

        // **TODO** type
        void define(std::string_view name, Callable &callable);

        // **TODO** type
        Value assign(const Token &name, Token::LiteralValue value, Token::Type op);

        // **TODO** type
        Value prefixIncDec(const Token &name, Token::Type op);

        // **TODO** type
        Value postfixIncDec(const Token &name, Token::Type op);

        // **TODO** type
        Value get(const Token &name) const;

    private:
        Environment *m_Enclosing;
        std::unordered_map<std::string_view, Value> m_Values;
    };

    Interpreter()
    :   m_Environment(nullptr)
    {
    }
    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    Token::LiteralValue evaluate(const Expression::Base *expression);
    void interpret(const Statement::StatementList &statements, Environment &environment);

    //---------------------------------------------------------------------------
    // Expression::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Expression::ArraySubscript &arraySubscript) final;
    virtual void visit(const Expression::Assignment &assignement) final;
    virtual void visit(const Expression::Binary &binary) final;
    virtual void visit(const Expression::Call &call) final;
    virtual void visit(const Expression::Cast &cast) final;
    virtual void visit(const Expression::Conditional &conditional) final;
    virtual void visit(const Expression::Grouping &grouping) final;
    virtual void visit(const Expression::Literal &literal) final;
    virtual void visit(const Expression::Logical &logical) final;
    virtual void visit(const Expression::PostfixIncDec &postfixIncDec) final;
    virtual void visit(const Expression::PrefixIncDec &postfixIncDec) final;
    virtual void visit(const Expression::Variable &variable) final;
    virtual void visit(const Expression::Unary &unary) final;
    
    //---------------------------------------------------------------------------
    // Statement::Visitor virtuals
    //---------------------------------------------------------------------------
    virtual void visit(const Statement::Break &breakStatement) final;
    virtual void visit(const Statement::Compound &compound) final;
    virtual void visit(const Statement::Continue &continueStatement) final;
    virtual void visit(const Statement::Do &doStatement) final;
    virtual void visit(const Statement::Expression &expression) final;
    virtual void visit(const Statement::For &forStatement) final;
    virtual void visit(const Statement::If &ifStatement) final;
    virtual void visit(const Statement::Labelled &labelled) final;
    virtual void visit(const Statement::Switch &switchStatement) final;
    virtual void visit(const Statement::VarDeclaration &varDeclaration) final;
    virtual void visit(const Statement::While &whileStatement) final;
    virtual void visit(const Statement::Print &print) final;

private:
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    Value m_Value;
    Value m_SwitchValue;
    
    Environment *m_Environment;
};
}