#include "parser.h"

// Standard C++ includes
#include <stdexcept>

// Standard C includes
#include <cassert>

// Mini-parse includes
#include "error_handler.h"

using namespace MiniParse;

//---------------------------------------------------------------------------
// Anonymous namespace
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// ParserState
//---------------------------------------------------------------------------
//! Class encapsulated logic to navigate through tokens
class ParserState
{
public:
    ParserState(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
        : m_Current(0), m_Tokens(tokens), m_ErrorHandler(errorHandler)
    {}

    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
    bool match(Token::Type t)
    {
        if(check(t)) {
            advance();
            return true;
        }
        else {
            return false;
        }
    }

    bool match(std::initializer_list<Token::Type> types) 
    {
        // Loop through types
        for(auto t : types) {
            if(match(t)) {
                return true;
            }
        }
        return false;
    }

    Token advance()
    {
        if(!isAtEnd()) {
            m_Current++;
        }

        return previous();
    }

    Token peek() const
    {
        return m_Tokens.at(m_Current);
    }

    Token previous() const
    {
        assert(m_Current > 0);
        return m_Tokens.at(m_Current - 1);
    }

    void error(std::string_view message) const
    {
        m_ErrorHandler.error(peek(), message);
    }

    bool check(Token::Type type) const
    {
        if(isAtEnd()) {
            return false;
        }
        else {
            return (peek().type == type);
        }
    }

    bool isAtEnd() const { return (peek().type == Token::Type::END_OF_FILE); }

    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    size_t m_Current;

    const std::vector<Token> &m_Tokens;

    ErrorHandler &m_ErrorHandler;
};

//---------------------------------------------------------------------------
// ParseError
//---------------------------------------------------------------------------
class ParseError
{
};

void synchronise(ParserState &parserState)
{
    parserState.advance();
    while(!parserState.isAtEnd()) {
        if(parserState.previous().type == Token::Type::SEMICOLON) {
            return;
        }

        switch(parserState.peek().type) {
        case Token::Type::FOR:
        case Token::Type::IF:
        case Token::Type::WHILE:
        case Token::Type::TYPE_SPECIFIER:
            return;
        }

        parserState.advance();
    }
}

// Helper to parse binary expressions
// **THINK I think this COULD be variadic but not clear if that's a good idea or not
template<typename N>
const Expression::Base *parseBinary(ParserState &parserState, N nonTerminal, std::initializer_list<Token::Type> types)
{
    auto *expression = nonTerminal(parserState);
    while(parserState.match(types)) {
        Token op = parserState.previous();
        expression = new Expression::Binary(expression, op, nonTerminal(parserState));
    }

    return expression;
}

const Expression::Base *parseExpression(ParserState &parserState);

const Expression::Base *parsePrimary(ParserState &parserState)
{
    // primary-expression ::=
    //      identifier              **TODO** 
    //      constant
    //      "(" expression ")"
    if(parserState.match(Token::Type::FALSE)) {
        return new Expression::Literal(false);
    }
    else if(parserState.match(Token::Type::TRUE)) {
        return new Expression::Literal(true);
    }
    if(parserState.match(Token::Type::NUMBER)) {
        return new Expression::Literal(parserState.previous().literalValue);
    }

    if(parserState.match(Token::Type::LEFT_PAREN)) {
        auto *expression = parseExpression(parserState);

        // If expression is followed by parenthesis
        if(parserState.match(Token::Type::RIGHT_PAREN)) {
            return new Expression::Grouping(expression);
        }
        // Otherwise, report error
        else {
            // **TODO** memory leak
            parserState.error("Expect ')' after expression");
            throw ParseError();
        }
    }

    parserState.error("Expect expression");
    throw ParseError();
}
const Expression::Base *parsePostfix(ParserState &parserState)
{
    // postfix-expression ::=
    //      primary-expression
    //      postfix-expression "[" expression "]"
    //      postfix-expression "(" argument-expression-list? ")"
    //      postfix-expression "." identifier
    //      postfix-expression "->" identifier
    //      postfix-expression "++"
    //      postfix-expression "--"
    //      "(" type-name ")" "{" initializer-list "}"
    //      "(" type-name ")" "{" initializer-list "," "}"
    return parsePrimary(parserState);
}


const Expression::Base *parseUnary(ParserState &parserState)
{
    // unary-expression ::=
    //      postfix-expression
    //      "++" unary-expression           **TODO** 
    //      "--" unary-expression           **TODO** 
    //      "+" cast-expression
    //      "-" cast-expression
    //      "~" cast-expression
    //      "!" cast-expression
    //      "sizeof" unary-expression       **TODO** 
    //      "sizeof" "(" type-name ")"      **TODO** 
    if(parserState.match({Token::Type::PLUS, Token::Type::MINUS, Token::Type::TILDA, Token::Type::NOT})) {
        Token op = parserState.previous();
        return new Expression::Unary(op, parseUnary(parserState));
    }

    return parsePostfix(parserState);
}

const Expression::Base *parseCast(ParserState &parserState)
{
    // cast-expression ::=
    //      unary-expression
    //      "(" type-name ")" cast-parseExpression
    return parseUnary(parserState);
}

const Expression::Base *parseMultiplicative(ParserState &parserState)
{
    // multiplicative-expression ::=
    //      cast-expression
    //      multiplicative-parseExpression "*" cast-parseExpression
    //      multiplicative-parseExpression "/" cast-parseExpression
    //      multiplicative-parseExpression "%" cast-parseExpression
    return parseBinary(parserState, parseCast, 
                       {Token::Type::STAR, Token::Type::SLASH, Token::Type::PERCENT});
}

const Expression::Base *parseAdditive(ParserState &parserState)
{
    // additive-expression ::=
    //      multiplicative-expression
    //      additive-parseExpression "+" multiplicative-parseExpression
    //      additive-parseExpression "-" multiplicative-parseExpression
    return parseBinary(parserState, parseMultiplicative, 
                       {Token::Type::MINUS, Token::Type::PLUS});
}

const Expression::Base *parseShift(ParserState &parserState)
{
    // shift-expression ::=
    //      additive-expression
    //      shift-parseExpression "<<" additive-parseExpression
    //      shift-parseExpression ">>" additive-parseExpression
    return parseAdditive(parserState);
}

const Expression::Base *parseRelational(ParserState &parserState)
{
    // relational-expression ::=
    //      shift-expression
    //      relational-parseExpression "<" shift-parseExpression
    //      relational-parseExpression ">" shift-parseExpression
    //      relational-parseExpression "<=" shift-parseExpression
    //      relational-parseExpression ">=" shift-parseExpression
    return parseBinary(parserState, parseShift, 
                       {Token::Type::GREATER, Token::Type::GREATER_EQUAL, 
                        Token::Type::LESS, Token::Type::LESS_EQUAL});
}

const Expression::Base *parseEquality(ParserState &parserState)
{
    // equality-expression ::=
    //      relational-expression
    //      equality-parseExpression "==" relational-parseExpression
    //      equality-parseExpression "!=" relational-parseExpression
    return parseBinary(parserState, parseRelational, 
                       {Token::Type::NOT_EQUAL, Token::Type::EQUAL_EQUAL});
}

const Expression::Base *parseExpression(ParserState &parserState)
{
    // expression ::=
    //      assignment-expression                   // **TODO**
    //      expression "," assignment-expression    // **TODO**
    return parseEquality(parserState);
}

const Statement::Base *parseExpressionStatement(ParserState &parserState)
{
    auto *expression = parseExpression(parserState);
    
    // If expression is followed by a semicolon, return new statement expression
    if(parserState.match(Token::Type::SEMICOLON)) {
        return new Statement::Expression(expression);
    }
    // Otherwise, report error
    else {
        // **TODO** memory leak
        parserState.error("Expect ';' after expression");
        throw ParseError();
    }
}
const Statement::Base *parseStatement(ParserState &parserState)
{
    // statement ::=
    //      labeled-statement       // **TODO**
    //      compound-statement      // **TODO**
    //      expression-statement
    //      selection-statement     // **TODO**
    //      iteration-statement     // **TODO**
    //      jump-statement          // **TODO**
    return parseExpressionStatement(parserState);
}
}


//---------------------------------------------------------------------------
// MiniParse::Parser
//---------------------------------------------------------------------------
namespace MiniParse::Parser
{
std::unique_ptr<const Expression::Base> parseTokens(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
{
    ParserState parserState(tokens, errorHandler);

    try {
        return std::unique_ptr<const Expression::Base>(parseExpression(parserState));
    }
    catch(ParseError &) {
        return nullptr;
    }
}
}