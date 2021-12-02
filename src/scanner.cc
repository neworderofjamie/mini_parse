#include "scanner.h"

// Standard C++ includes
#include <charconv>
#include <unordered_map>
#include <unordered_set>

// Standard C includes
#include <cctype>

using namespace Parser;
using namespace Parser::Scanner;


//---------------------------------------------------------------------------
// Anonymous namespace
//---------------------------------------------------------------------------
namespace
{
const std::unordered_map<std::string_view, Token::Type> keywords{
    {"const", Token::Type::CONST},
    {"do", Token::Type::DO},
    {"else", Token::Type::ELSE},
    {"false", Token::Type::FALSE},
    {"for", Token::Type::FOR},
    {"if", Token::Type::IF},
    {"true", Token::Type::TRUE},
    {"while", Token::Type::WHILE},
    // **TEMP** just map types to tokens here too
    {"char", Token::Type::TYPE_SPECIFIER},
    {"short", Token::Type::TYPE_SPECIFIER},
    {"int", Token::Type::TYPE_SPECIFIER},
    {"long", Token::Type::TYPE_SPECIFIER},
    {"scalar", Token::Type::TYPE_SPECIFIER},
    {"float", Token::Type::TYPE_SPECIFIER},
    {"double", Token::Type::TYPE_SPECIFIER},
    {"signed", Token::Type::TYPE_SPECIFIER},
    {"unsigned", Token::Type::TYPE_SPECIFIER},
    {"bool", Token::Type::TYPE_SPECIFIER}};

class Cursor
{
public:
    Cursor(std::string_view source)
        : m_Start(0), m_Current(0), m_Source(source), m_Line(1)
    {}

    //---------------------------------------------------------------------------
    // Public API
    //---------------------------------------------------------------------------
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
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    size_t m_Start;
    size_t m_Current;
    size_t m_Line;

    const std::string_view m_Source;
};

bool isodigit(char c)
{
    return (c >= '0' && c <= '7');
}

template<typename T>
T toCharsThrow(std::string_view input, std::chars_format format = std::chars_format::general)
{
    T out;
    const auto result = std::from_chars(input.data(), input.data() + input.size(), out);
    if(result.ec == std::errc::invalid_argument) {
        throw std::invalid_argument("Unable to convert chars '" + std::string{input} + "'");
    }
    else if(result.ec == std::errc::result_out_of_range) {
        throw std::out_of_range("Unable to convert chars '" + std::string{input} + "'");
    }
    return out;
}

void emplaceToken(std::vector<Token> &tokens, Token::Type type, const Cursor &cursor, Token::LiteralValue literalValue = Token::LiteralValue())
{
    tokens.emplace_back(type, cursor.getLexeme(), cursor.getLine(), literalValue);
}

void scanIntegerSuffix(Cursor &cursor)
{
    // Read suffix
    // **TODO** complete
    while(std::tolower(cursor.peek()) == 'u' || std::tolower(cursor.peek()) == 'l') {
        cursor.advance();
    }
}

void scanNumber(Cursor &cursor, std::vector<Token> &tokens) 
{
    // If this is a hexadecimal literal
    if(cursor.peek() == '0' && std::tolower(cursor.peekNext()) == 'x') {
        // Advance past
        cursor.advance();
        cursor.advance();

        // Read hexadecimal digits
        while(std::isxdigit(cursor.peek())) {
            cursor.advance();
        }

        // Read decimal place
        bool isFloat = false;
        if(cursor.peek() == '.') {
            isFloat = true;
            cursor.advance();
        }

        // Read hexadecimal digits
        while(std::isxdigit(cursor.peek())) {
            cursor.advance();
        }

        // If number is float
        if(isFloat) {
            // Check there's an exponent as these are REQUIRED for floating point literals
            if(cursor.peek() != 'p') {
                throw Error(cursor.getLine(), "Hexadecimal floating point literal missing exponent.");
            }
            else {
                // Read p
                cursor.advance();

                // Read sign
                if(cursor.peek() == '-' || cursor.peek() == '+') {
                    cursor.advance();
                }

                // Read DECIMAL digits
                while(std::isdigit(cursor.peek())) {
                    cursor.advance();
                }

                // If literal has floating point suffix
                const char suffix = std::tolower(cursor.peek());
                if(suffix == 'f') {
                    // Add single-precision token
                    // **NOTE** skip 0x prefix
                    emplaceToken(tokens, Token::Type::NUMBER, cursor,
                                 toCharsThrow<float>(cursor.getLexeme().substr(2), std::chars_format::hex));

                    // Advance
                    // **NOTE** we do this AFTER parsing float as std::to_chars doesn't deal with suffixes
                    cursor.advance();
                }
                // Add double-precision token
                // **NOTE** skip 0x prefix
                else {
                    emplaceToken(tokens, Token::Type::NUMBER, cursor,
                                 toCharsThrow<double>(cursor.getLexeme().substr(2), std::chars_format::hex));
                }
            }
        }
        // Otherwise, number is hexadecimal integer
        else {
            scanIntegerSuffix(cursor);

            // Add integer token
            // **TODO** different types
            // **NOTE** skip 0x prefix
            emplaceToken(tokens, Token::Type::NUMBER, cursor,
                         toCharsThrow<int64_t>(cursor.getLexeme().substr(2), std::chars_format::hex));
        }
    }
    // Otherwise, if this is an octal integer
    else if(cursor.peek() == '0' && isodigit(cursor.peekNext())){
        throw ErrorUnsupported(cursor.getLine(), "Octal literals unsupported.");
    }
    // Otherwise, if it's decimal
    else {
        // Read digits
        while(std::isdigit(cursor.peek())) {
            cursor.advance();
        }

        // Read decimal place
        bool isFloat = false;
        if(cursor.peek() == '.') {
            isFloat = true;
            cursor.advance();
        }

        // Read digits
        while(std::isdigit(cursor.peek())) {
            cursor.advance();
        }

        // If it's float
        if(isFloat) {
            // If there's an exponent
            if(cursor.peek() != 'e') {
                // Read e
                cursor.advance();

                // Read sign
                if(cursor.peek() == '-' || cursor.peek() == '+') {
                    cursor.advance();
                }

                // Read digits
                while(std::isdigit(cursor.peek())) {
                    cursor.advance();
                }

                // If literal has floating point suffix
                const char suffix = std::tolower(cursor.peek());
                if(suffix == 'f') {
                    // Add single-precision token
                    emplaceToken(tokens, Token::Type::NUMBER, cursor,
                                 toCharsThrow<float>(cursor.getLexeme()));

                    // Advance
                    // **NOTE** we do this AFTER parsing float as std::to_chars doesn't deal with suffixes
                    cursor.advance();
                }
                // Otherwise, add double-precision token
                else {
                    emplaceToken(tokens, Token::Type::NUMBER, cursor,
                                 toCharsThrow<double>(cursor.getLexeme()));
                }
            }
        }
        // Otherwise, number is integer
        else {
            scanIntegerSuffix(cursor);

            // Add integer token
            // **TODO** different types
            emplaceToken(tokens, Token::Type::NUMBER, cursor,
                         toCharsThrow<int64_t>(cursor.getLexeme()));
        }
    }
}

void scanIdentifier(Cursor &cursor, std::vector<Token> &tokens)
{
    // Read subsequent alphanumeric characters and underscores
    while(std::isalnum(cursor.peek()) || cursor.peek() == '_') {
        cursor.advance();
    }

    // If identifier is a keyword, add appropriate token
    const auto k = keywords.find(cursor.getLexeme());
    if(k != keywords.cend()) {
        emplaceToken(tokens, k->second, cursor);
    }
    // Otherwise, add identifier token
    else {
        emplaceToken(tokens, Token::Type::IDENTIFIER, cursor);
    }
}

void scanToken(Cursor &cursor, std::vector<Token> &tokens)
{
    using namespace Parser;

    char c = cursor.advance();
    switch(c) {
        // Single character tokens
        case '(': emplaceToken(tokens, Token::Type::LEFT_PAREN, cursor); break;
        case ')': emplaceToken(tokens, Token::Type::RIGHT_PAREN, cursor); break;
        case '{': emplaceToken(tokens, Token::Type::LEFT_BRACE, cursor); break;
        case '}': emplaceToken(tokens, Token::Type::RIGHT_BRACE, cursor); break;
        case ',': emplaceToken(tokens, Token::Type::COMMA, cursor); break;
        case '.': emplaceToken(tokens, Token::Type::DOT, cursor); break;
        case '-': emplaceToken(tokens, Token::Type::MINUS, cursor); break;
        case '+': emplaceToken(tokens, Token::Type::PLUS, cursor); break;
        case ';': emplaceToken(tokens, Token::Type::SEMICOLON, cursor); break;
        case '*': emplaceToken(tokens, Token::Type::STAR, cursor); break;

        // Operators
        case '!': emplaceToken(tokens, cursor.match('=') ? Token::Type::NOT_EQUAL : Token::Type::NOT, cursor); break;
        case '=': emplaceToken(tokens, cursor.match('=') ? Token::Type::EQUAL_EQUAL : Token::Type::EQUAL, cursor); break;
        case '<': emplaceToken(tokens, cursor.match('=') ? Token::Type::LESS_EQUAL : Token::Type::LESS, cursor); break;
        case '>': emplaceToken(tokens, cursor.match('=') ? Token::Type::GREATER_EQUAL : Token::Type::GREATER, cursor); break;

        case '/':
        {
            // Line comment
            if(cursor.match('/')) {
                while(cursor.peek() != '\n' && !cursor.isAtEnd()) {
                    cursor.advance();
                }
            }
            else {
                emplaceToken(tokens, Token::Type::SLASH, cursor);
            }
            break;
        }

        // Whitespace
        case ' ':
        case '\r':
        case '\t':
            break;

        // New line
        case '\n': cursor.nextLine(); break;

        default:
        {
            // If we have a digit or a period, scan number
            if(std::isdigit(c) || c == '.') {
                scanNumber(cursor, tokens);
            }
            // Otherwise, if 
            else if(c == '$') {

            }
            // Otherwise, scan identifier
            else if(std::isalpha(c) || c == '_') {
                scanIdentifier(cursor, tokens);
            }
            else {
                throw Error(cursor.getLine(), "Unexpected character.");
            }
        }
    }
}
}
//---------------------------------------------------------------------------
// Parser::Scanner
//---------------------------------------------------------------------------
namespace Parser::Scanner
{
std::vector<Token> scanTokens(const std::string_view &source)
{
    std::vector<Token> tokens;

    Cursor cursor(source);

    // Current line
    size_t line = 1;

    // Scan tokens
    while(!cursor.isAtEnd()) {
        cursor.resetLexeme();
        scanToken(cursor, tokens);
    }

    emplaceToken(tokens, Token::Type::END_OF_FILE, cursor);
    return tokens;
}
}