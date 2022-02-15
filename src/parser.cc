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
// ParseError
//---------------------------------------------------------------------------
class ParseError
{
};

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

    void error(Token token, std::string_view message) const
    {
        m_ErrorHandler.error(token, message);
    }

    Token consume(Token::Type type, std::string_view message) 
    {
        if(check(type)) {
            return advance();
        }

        error(message);
        throw ParseError();
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
private:
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    size_t m_Current;

    const std::vector<Token> &m_Tokens;

    ErrorHandler &m_ErrorHandler;
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

// Forward declarations
Expression::ExpressionPtr parseExpression(ParserState &parserState);
Statement::StatementPtr parseBlockItem(ParserState &parserState);
Statement::StatementPtr parseStatement(ParserState &parserState);

// Helper to parse binary expressions
// **THINK I think this COULD be variadic but not clear if that's a good idea or not
template<typename N>
Expression::ExpressionPtr parseBinary(ParserState &parserState, N nonTerminal, std::initializer_list<Token::Type> types)
{
    auto expression = nonTerminal(parserState);
    while(parserState.match(types)) {
        Token op = parserState.previous();
        expression = std::make_unique<Expression::Binary>(std::move(expression), op, nonTerminal(parserState));
    }

    return expression;
}

Expression::ExpressionPtr parsePrimary(ParserState &parserState)
{
    // primary-expression ::=
    //      identifier
    //      constant
    //      "(" expression ")"
    if(parserState.match(Token::Type::FALSE)) {
        return std::make_unique<Expression::Literal>(false);
    }
    else if(parserState.match(Token::Type::TRUE)) {
        return std::make_unique<Expression::Literal>(true);
    }
    else if(parserState.match(Token::Type::NUMBER)) {
        return std::make_unique<Expression::Literal>(parserState.previous().literalValue);
    }
    else if(parserState.match(Token::Type::IDENTIFIER)) {
        return std::make_unique<Expression::Variable>(parserState.previous());
    }
    else if(parserState.match(Token::Type::LEFT_PAREN)) {
        auto expression = parseExpression(parserState);

        parserState.consume(Token::Type::RIGHT_PAREN, "Expect ')' after expression");
        return std::make_unique<Expression::Grouping>(std::move(expression));
    }

    parserState.error("Expect expression");
    throw ParseError();
}

Expression::ExpressionPtr parsePostfix(ParserState &parserState)
{
    // postfix-expression ::=
    //      primary-expression
    //      postfix-expression "[" expression "]"
    //      postfix-expression "(" argument-expression-list? ")"
    //      postfix-expression "++"
    //      postfix-expression "--"
    //      "(" type-name ")" "{" initializer-list "}"
    //      "(" type-name ")" "{" initializer-list "," "}"
    return parsePrimary(parserState);
}


Expression::ExpressionPtr parseUnary(ParserState &parserState)
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
        return std::make_unique<Expression::Unary>(op, parseUnary(parserState));
    }

    return parsePostfix(parserState);
}

Expression::ExpressionPtr parseCast(ParserState &parserState)
{
    // cast-expression ::=
    //      unary-expression
    //      "(" type-name ")" cast-parseExpression
    return parseUnary(parserState);
}

Expression::ExpressionPtr parseMultiplicative(ParserState &parserState)
{
    // multiplicative-expression ::=
    //      cast-expression
    //      multiplicative-parseExpression "*" cast-parseExpression
    //      multiplicative-parseExpression "/" cast-parseExpression
    //      multiplicative-parseExpression "%" cast-parseExpression
    return parseBinary(parserState, parseCast, 
                       {Token::Type::STAR, Token::Type::SLASH, Token::Type::PERCENT});
}

Expression::ExpressionPtr parseAdditive(ParserState &parserState)
{
    // additive-expression ::=
    //      multiplicative-expression
    //      additive-parseExpression "+" multiplicative-parseExpression
    //      additive-parseExpression "-" multiplicative-parseExpression
    return parseBinary(parserState, parseMultiplicative, 
                       {Token::Type::MINUS, Token::Type::PLUS});
}

Expression::ExpressionPtr parseShift(ParserState &parserState)
{
    // shift-expression ::=
    //      additive-expression
    //      shift-parseExpression "<<" additive-parseExpression
    //      shift-parseExpression ">>" additive-parseExpression
    return parseAdditive(parserState);
}

Expression::ExpressionPtr parseRelational(ParserState &parserState)
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

Expression::ExpressionPtr parseEquality(ParserState &parserState)
{
    // equality-expression ::=
    //      relational-expression
    //      equality-parseExpression "==" relational-parseExpression
    //      equality-parseExpression "!=" relational-parseExpression
    return parseBinary(parserState, parseRelational, 
                       {Token::Type::NOT_EQUAL, Token::Type::EQUAL_EQUAL});
}

Expression::ExpressionPtr parseLogicalAnd(ParserState &parserState)
{
    // logical-AND-expression ::=
    //      inclusive-OR-expression
    //      logical-AND-expression "&&" inclusive-OR-expression
    // **THINK** parseLogicalAnd here (obviously) stack-overflows - why is this the grammar?
    auto expression = parseEquality(parserState);

    while(parserState.match(Token::Type::AMPERSAND_AMPERSAND)) {
        Token op = parserState.previous();
        auto right = parseEquality(parserState);
        expression = std::make_unique<Expression::Logical>(std::move(expression), op, std::move(right));
    }
    return expression;
}

Expression::ExpressionPtr parseLogicalOr(ParserState &parserState)
{
    // logical-OR-expression ::=
    //      logical-AND-expression
    //      logical-OR-expression "||" logical-AND-expression
    // **THINK** parseLogicalOr here (obviously) stack-overflows - why is this the grammar?
    auto expression = parseLogicalAnd(parserState);

    while(parserState.match(Token::Type::PIPE_PIPE)) {
        Token op = parserState.previous();
        auto right = parseLogicalAnd(parserState);
        expression = std::make_unique<Expression::Logical>(std::move(expression), op, std::move(right));
    }
    return expression;
}

Expression::ExpressionPtr parseConditional(ParserState &parserState)
{
    // conditional-expression ::=
    //      logical-OR-expression
    //      logical-OR-expression "?" expression ":" conditional-expression
    return parseLogicalOr(parserState);
}

Expression::ExpressionPtr parseAssignment(ParserState &parserState)
{
    // assignment-expression ::=
    //      conditional-expression
    //      unary-expression assignment-operator assignment-expression
    auto expression = parseConditional(parserState);
    if(parserState.match({Token::Type::EQUAL, Token::Type::STAR_EQUAL, Token::Type::SLASH_EQUAL, 
                          Token::Type::PERCENT_EQUAL, Token::Type::PLUS_EQUAL, Token::Type::MINUS_EQUAL, 
                          Token::Type::AMPERSAND_EQUAL, Token::Type::CARET_EQUAL, Token::Type::PIPE_EQUAL})) 
    {
        Token op = parserState.previous();
        auto value = parseAssignment(parserState);

        // **TODO** everything all the way up(?) from unary are l-value so can be used - not just variable
        auto expressionVariable = dynamic_cast<const Expression::Variable*>(expression.get());
        if(expressionVariable != nullptr) {
            return std::make_unique<Expression::Assignment>(expressionVariable->getName(), op, std::move(value));
        }
        else {
            parserState.error(op, "Invalid assignment target");
        }
    }

    return expression;
}

Expression::ExpressionPtr parseExpression(ParserState &parserState)
{
    // expression ::=
    //      assignment-expression                   // **TODO**
    //      expression "," assignment-expression    // **TODO**
    return parseAssignment(parserState);
}

Statement::StatementPtr parseCompoundStatement(ParserState &parserState)
{
    // compound-statement ::=
    //      "{" block-item-list? "}"
    // block-item-list ::=
    //      block-item
    //      block-item-list block-item
    // block-item ::=
    //      declaration
    //      statement
    Statement::StatementList statements;
    while(!parserState.check(Token::Type::RIGHT_BRACE) && !parserState.isAtEnd()) {
        statements.emplace_back(parseBlockItem(parserState));
    }
    parserState.consume(Token::Type::RIGHT_BRACE, "Expect '}' after compound statement.");

    return std::make_unique<Statement::Compound>(std::move(statements));
}

Statement::StatementPtr parseExpressionStatement(ParserState &parserState)
{
    //  expression-statement ::=
    //      expression? ";"
    auto expression = parseExpression(parserState);
    
    parserState.consume(Token::Type::SEMICOLON, "Expect ';' after expression");
    return std::make_unique<Statement::Expression>(std::move(expression));
}

Statement::StatementPtr parsePrintStatement(ParserState &parserState)
{
    auto expression = parseExpression(parserState);

    parserState.consume(Token::Type::SEMICOLON, "Expect ';' after expression");
    return std::make_unique<Statement::Print>(std::move(expression));
}

Statement::StatementPtr parseSelectionStatement(ParserState &parserState)
{
    // selection-statement ::=
    //      "if" "(" expression ")" statement
    //      "if" "(" expression ")" statement "else" statement
    //      "switch" "(" expression ")" statement           // **TODO**

    parserState.consume(Token::Type::LEFT_PAREN, "Expect '(' after 'if'");
    auto condition = parseExpression(parserState);
    parserState.consume(Token::Type::RIGHT_PAREN, "Expect ')' after 'if'");

    auto thenBranch = parseStatement(parserState);
    Statement::StatementPtr elseBranch;
    if(parserState.match(Token::Type::ELSE)) {
        elseBranch = parseStatement(parserState);
    }

    return std::make_unique<Statement::If>(std::move(condition), 
                                           std::move(thenBranch), 
                                           std::move(elseBranch));
}

Statement::StatementPtr parseStatement(ParserState &parserState)
{
    // statement ::=
    //      labeled-statement       // **TODO**
    //      compound-statement
    //      expression-statement
    //      print-statement         // **TEMP**
    //      selection-statement     
    //      iteration-statement     // **TODO**
    //      jump-statement          // **TODO**
    if(parserState.match(Token::Type::PRINT)) {
        return parsePrintStatement(parserState);
    }
    else if(parserState.match(Token::Type::IF)) {
        return parseSelectionStatement(parserState);
    }
    else if(parserState.match(Token::Type::LEFT_BRACE)) {
        return parseCompoundStatement(parserState);
    }
    else {
        return parseExpressionStatement(parserState);
    }
}

std::unique_ptr<const Statement::Base> parseDeclaration(ParserState &parserState)
{
    // declaration ::=
    //      declaration-specifiers init-declarator-list? ";"

    // declaration-specifiers ::=
    //      declaration-specifiers?
    //      type-specifier declaration-specifiers?
    //      type-qualifier declaration-specifiers?

    // type-specifier ::=
    //      "char"
    //      "short"
    //      "int"
    //      "long"
    //      "float"
    //      "double"
    //      "signed"
    //      "unsigned"
    //      "bool"
    //      typedef-name    // **TODO** not sure how to address ambiguity with subsequent identifier

    // type-qualifier ::=
    //      "const"

    // Build declaration specifier list (single list of specifiers and qualifiers is fine for our purposes)
    std::vector<Token> declarationSpecifiers{parserState.previous()};
    while(parserState.match({Token::Type::TYPE_QUALIFIER, Token::Type::TYPE_SPECIFIER})) {
        declarationSpecifiers.push_back(parserState.previous());
    }

    // Read init declarator list
    std::vector<std::tuple<Token, Expression::ExpressionPtr>> initDeclaratorList;
    do {
        // init-declarator-list ::=
        //      init-declarator
        //      init-declarator-list "," init-declarator

        // init-declarator ::=
        //      declarator
        //      declarator "=" assignment-expression

        // declarator ::=
        //      identifier
        Token identifier = parserState.consume(Token::Type::IDENTIFIER, "Expect variable name");
        Expression::ExpressionPtr initialiser;
        if(parserState.match(Token::Type::EQUAL)) {
            initialiser = parseAssignment(parserState);
        }
        initDeclaratorList.emplace_back(identifier, std::move(initialiser));
    } while(!parserState.isAtEnd() && parserState.match(Token::Type::COMMA));

    parserState.consume(Token::Type::SEMICOLON, "Expect ';' after variable declaration");
    return std::make_unique<Statement::VarDeclaration>(std::move(declarationSpecifiers), 
                                                       std::move(initDeclaratorList));
}

std::unique_ptr<const Statement::Base> parseBlockItem(ParserState &parserState)
{
    // block-item ::=
    //      declaration
    //      statement
    try {
        if(parserState.match({Token::Type::TYPE_SPECIFIER, Token::Type::TYPE_QUALIFIER})) {
            return parseDeclaration(parserState);
        }
        else {
            return parseStatement(parserState);
        }
    }
    catch(ParseError &) {
        synchronise(parserState);
        return nullptr;
    }
}
}   // Anonymous namespace


//---------------------------------------------------------------------------
// MiniParse::Parser
//---------------------------------------------------------------------------
namespace MiniParse::Parser
{
Expression::ExpressionPtr parseExpression(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
{
    ParserState parserState(tokens, errorHandler);

    try {
        return parseExpression(parserState);
    }
    catch(ParseError &) {
        return nullptr;
    }
}

Statement::StatementList parseStatements(const std::vector<Token> &tokens, ErrorHandler &errorHandler)
{
    ParserState parserState(tokens, errorHandler);
    std::vector<std::unique_ptr<const Statement::Base>> statements;

    while(!parserState.isAtEnd()) {
        statements.emplace_back(parseBlockItem(parserState));
    }
    return statements;
}
}