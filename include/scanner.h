#pragma once

// Standard C++ includes
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Standard C includes
#include <cstdint>

// ---------------------------------------------------------------------------
// Parser::Cursor
// ---------------------------------------------------------------------------
namespace Parser
{
class Cursor
{
public:
    Cursor(std::string_view source)
    :   m_Start(0), m_Current(0), m_Source(source), m_Line(1)
    {}

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------
    char advance() {
        m_Current++;
        return m_Source.at(m_Current - 1);
    }

    bool match(char expected) 
    {
        if(isAtEnd()) {
            return false;
        }
        if(m_Source.at(m_Current) != expected) {
            return false;
        }

        m_Current++;
        return true;
    }

    void resetLexeme()
    {
        m_Start = m_Current;
    }

    char peek() const
    {
        if(isAtEnd()) {
            return '\0';
        }
        return m_Source.at(m_Current);
    }

    char peekNext() const
    {
        if((m_Current + 1) >= m_Source.length()) {
            return '\0';
        }
        else {
            return m_Source.at(m_Current + 1);
        }
    }

    std::string_view getLexeme() const
    {
        return m_Source.substr(m_Start, m_Current - m_Start);
    }

    size_t getLine() const { return m_Line; }

    bool isAtEnd() const { return m_Current >= m_Source.length(); }

    void nextLine() { m_Line++; }

private:
    // ---------------------------------------------------------------------------
    // Members
    // ---------------------------------------------------------------------------
    size_t m_Start;
    size_t m_Current;
    size_t m_Line;

    const std::string_view m_Source;
};

// ---------------------------------------------------------------------------
// Parser::Token
// ---------------------------------------------------------------------------
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
        INT_IDENTIFIER, EXT_IDENTIFIER, NUMBER,

        // Types
        TYPE,

        // Keywords
        CONST, DO, ELSE, FALSE, FOR, IF, TRUE, WHILE,

        END_OF_FILE,
    };

    Token(Type type, const Cursor &cursor, LiteralValue literalValue = LiteralValue())
        : type(type), lexeme(cursor.getLexeme()), line(cursor.getLine()), literalValue(literalValue)
    {
    }


    const Type type;
    const std::string_view lexeme;
    const size_t line;
    const LiteralValue literalValue;
};

// ---------------------------------------------------------------------------
// Parser::ScannerError
// ---------------------------------------------------------------------------
class ScannerError : public std::runtime_error
{
public:
    ScannerError(size_t line, const std::string &message) 
    :   line(line), std::runtime_error(message)
    {}

    const size_t line;
};

// ---------------------------------------------------------------------------
// Parser::ScannerErrorUnsupported
// ---------------------------------------------------------------------------
class ScannerErrorUnsupported : public ScannerError
{
public:
    ScannerErrorUnsupported(size_t line, const std::string &message) 
    :   ScannerError(line, message)
    {}
};

std::vector<Token> scanTokens(const std::string_view &source);

}