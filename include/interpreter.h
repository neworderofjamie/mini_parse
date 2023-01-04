#pragma once

// Standard C++ includes
#include <optional>
#include <unordered_map>
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "statement.h"

//---------------------------------------------------------------------------
// MiniParse::Interpreter::Callable
//---------------------------------------------------------------------------
namespace MiniParse::Interpreter
{
class Callable
{
public:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual std::optional<size_t> getArity() const = 0;
    virtual Token::LiteralValue call(const std::vector<Token::LiteralValue> &arguments) = 0;
};

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

    //------------------------------------------------------------------------
    // Typedefines
    //------------------------------------------------------------------------
    typedef std::variant<Token::LiteralValue, std::reference_wrapper<Callable>> Value;

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
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
    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    Environment *m_Enclosing;
    std::unordered_map<std::string_view, Value> m_Values;
};

//---------------------------------------------------------------------------
// Free functions
//---------------------------------------------------------------------------
void interpret(const Statement::StatementList &statements, Environment &environment);
}