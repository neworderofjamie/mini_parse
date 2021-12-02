#pragma once

// Standard C++ includes
#include <string_view>
#include <variant>

// Standard C includes
#include <cstdint>

//---------------------------------------------------------------------------
// Parser::Token
//---------------------------------------------------------------------------
namespace Parser
{
struct Token
{
    typedef std::variant<std::monostate, bool, float, double, uint32_t, int32_t, uint64_t, int64_t> LiteralValue;

    enum class Type
    {
        // Single-character tokens
        LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
        COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

        // One or two character tokens
        NOT, NOT_EQUAL,
        EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,

        // Literals   
        IDENTIFIER, NUMBER,

        // Types
        TYPE_SPECIFIER,

        // Keywords
        CONST, DO, ELSE, FALSE, FOR, IF, TRUE, WHILE,

        END_OF_FILE,
    };

    Token(Type type, std::string_view lexeme, size_t line, LiteralValue literalValue = LiteralValue())
        : type(type), lexeme(lexeme), line(line), literalValue(literalValue)
    {
    }

    const Type type;
    const std::string_view lexeme;
    const size_t line;
    const LiteralValue literalValue;
};

}
