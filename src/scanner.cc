#include "scanner.h"

// Standard C++ includes
#include <charconv>
#include <unordered_map>
#include <unordered_set>

// Standard C includes
#include <cctype>

// Mini-parse includes
#include "error_handler.h"

using namespace MiniParse;
using namespace MiniParse::Scanner;

//---------------------------------------------------------------------------
// Anonymous namespace
//---------------------------------------------------------------------------
namespace
{
const std::unordered_map<std::string_view, Token::Type> keywords{
    {"const", Token::Type::TYPE_QUALIFIER},
    {"do", Token::Type::DO},
    {"else", Token::Type::ELSE},
    {"false", Token::Type::FALSE},
    {"for", Token::Type::FOR},
    {"if", Token::Type::IF},
    {"true", Token::Type::TRUE},
    {"while", Token::Type::WHILE},
    {"switch", Token::Type::SWITCH},
    {"print", Token::Type::PRINT},  // **HACK**
    {"char", Token::Type::TYPE_SPECIFIER},
    {"short", Token::Type::TYPE_SPECIFIER},
    {"int", Token::Type::TYPE_SPECIFIER},
    {"long", Token::Type::TYPE_SPECIFIER},
    {"float", Token::Type::TYPE_SPECIFIER},
    {"double", Token::Type::TYPE_SPECIFIER},
    {"signed", Token::Type::TYPE_SPECIFIER},
    {"unsigned", Token::Type::TYPE_SPECIFIER},
    {"bool", Token::Type::TYPE_SPECIFIER}};

//---------------------------------------------------------------------------
// ScanState
//---------------------------------------------------------------------------
//! Class encapsulated logic to navigate through source characters
class ScanState
{
public:
    ScanState(std::string_view source, ErrorHandler &errorHandler)
        : m_Start(0), m_Current(0), m_Source(source), m_Line(1), m_ErrorHandler(errorHandler)
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

    void error(std::string_view message)
    {
        m_ErrorHandler.error(getLine(), message);
    }
private:
    //---------------------------------------------------------------------------
    // Members
    //---------------------------------------------------------------------------
    size_t m_Start;
    size_t m_Current;
    size_t m_Line;

    const std::string_view m_Source;

    ErrorHandler &m_ErrorHandler;
};

bool isodigit(char c)
{
    return (c >= '0' && c <= '7');
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void emplaceToken(std::vector<Token> &tokens, Token::Type type, const ScanState &scanState, Token::LiteralValue literalValue = Token::LiteralValue())
{
    tokens.emplace_back(type, scanState.getLexeme(), scanState.getLine(), literalValue);
}
//---------------------------------------------------------------------------
void scanIntegerSuffix(ScanState &scanState)
{
    // Read suffix
    // **TODO** complete
    while(std::tolower(scanState.peek()) == 'u' || std::tolower(scanState.peek()) == 'l') {
        scanState.advance();
    }
}
//---------------------------------------------------------------------------
void scanNumber(char c, ScanState &scanState, std::vector<Token> &tokens) 
{
    // If this is a hexadecimal literal
    if(c == '0' && (scanState.match('x') || scanState.match('X'))) {
        // Read hexadecimal digits
        while(std::isxdigit(scanState.peek())) {
            scanState.advance();
        }

        // Read decimal place
        bool isFloat = false;
        if(scanState.peek() == '.') {
            isFloat = true;
            scanState.advance();
        }

        // Read hexadecimal digits
        while(std::isxdigit(scanState.peek())) {
            scanState.advance();
        }

        // If number is float
        if(isFloat) {
            // Check there's an exponent as these are REQUIRED for floating point literals
            if(scanState.peek() != 'p') {
                scanState.error("Hexadecimal floating point literal missing exponent.");
            }
            else {
                // Read p
                scanState.advance();

                // Read sign
                if(scanState.peek() == '-' || scanState.peek() == '+') {
                    scanState.advance();
                }

                // Read DECIMAL digits
                while(std::isdigit(scanState.peek())) {
                    scanState.advance();
                }

                // If literal has floating point suffix
                const char suffix = std::tolower(scanState.peek());
                if(suffix == 'f') {
                    // Add single-precision token
                    // **NOTE** skip 0x prefix
                    emplaceToken(tokens, Token::Type::NUMBER, scanState,
                                 toCharsThrow<float>(scanState.getLexeme().substr(2), std::chars_format::hex));

                    // Advance
                    // **NOTE** we do this AFTER parsing float as std::to_chars doesn't deal with suffixes
                    scanState.advance();
                }
                // Add double-precision token
                // **NOTE** skip 0x prefix
                else {
                    emplaceToken(tokens, Token::Type::NUMBER, scanState,
                                 toCharsThrow<double>(scanState.getLexeme().substr(2), std::chars_format::hex));
                }
            }
        }
        // Otherwise, number is hexadecimal integer
        else {
            scanIntegerSuffix(scanState);

            // Add integer token
            // **TODO** different types
            // **NOTE** skip 0x prefix
            emplaceToken(tokens, Token::Type::NUMBER, scanState,
                         toCharsThrow<int64_t>(scanState.getLexeme().substr(2), std::chars_format::hex));
        }
    }
    // Otherwise, if this is an octal integer
    else if(c == '0' && isodigit(scanState.peekNext())){
        scanState.error("Octal literals unsupported.");
    }
    // Otherwise, if it's decimal
    else {
        // Read digits
        while(std::isdigit(scanState.peek())) {
            scanState.advance();
        }

        // Read decimal place
        bool isFloat = false;
        if(scanState.peek() == '.') {
            isFloat = true;
            scanState.advance();
        }

        // Read digits
        while(std::isdigit(scanState.peek())) {
            scanState.advance();
        }

        // If it's float
        if(isFloat) {
            // If there's an exponent
            if(scanState.peek() != 'e') {
                // Read e
                scanState.advance();

                // Read sign
                if(scanState.peek() == '-' || scanState.peek() == '+') {
                    scanState.advance();
                }

                // Read digits
                while(std::isdigit(scanState.peek())) {
                    scanState.advance();
                }

                // If literal has floating point suffix
                const char suffix = std::tolower(scanState.peek());
                if(suffix == 'f') {
                    // Add single-precision token
                    emplaceToken(tokens, Token::Type::NUMBER, scanState,
                                 toCharsThrow<float>(scanState.getLexeme()));

                    // Advance
                    // **NOTE** we do this AFTER parsing float as std::to_chars doesn't deal with suffixes
                    scanState.advance();
                }
                // Otherwise, add double-precision token
                else {
                    emplaceToken(tokens, Token::Type::NUMBER, scanState,
                                 toCharsThrow<double>(scanState.getLexeme()));
                }
            }
        }
        // Otherwise, number is integer
        else {
            scanIntegerSuffix(scanState);

            // Add integer token
            // **TODO** different types
            emplaceToken(tokens, Token::Type::NUMBER, scanState,
                         toCharsThrow<int64_t>(scanState.getLexeme()));
        }
    }
}
//---------------------------------------------------------------------------
void scanIdentifier(ScanState &scanState, std::vector<Token> &tokens)
{
    // Read subsequent alphanumeric characters and underscores
    while(std::isalnum(scanState.peek()) || scanState.peek() == '_') {
        scanState.advance();
    }

    // If identifier is a keyword, add appropriate token
    const auto k = keywords.find(scanState.getLexeme());
    if(k != keywords.cend()) {
        emplaceToken(tokens, k->second, scanState);
    }
    // Otherwise, add identifier token
    else {
        emplaceToken(tokens, Token::Type::IDENTIFIER, scanState);
    }
}
//---------------------------------------------------------------------------
void scanToken(ScanState &scanState, std::vector<Token> &tokens)
{
    using namespace MiniParse;

    char c = scanState.advance();
    switch(c) {
        // Single character tokens
        case '(': emplaceToken(tokens, Token::Type::LEFT_PAREN, scanState); break;
        case ')': emplaceToken(tokens, Token::Type::RIGHT_PAREN, scanState); break;
        case '{': emplaceToken(tokens, Token::Type::LEFT_BRACE, scanState); break;
        case '}': emplaceToken(tokens, Token::Type::RIGHT_BRACE, scanState); break;
        case ',': emplaceToken(tokens, Token::Type::COMMA, scanState); break;
        case '.': emplaceToken(tokens, Token::Type::DOT, scanState); break;
        case ':': emplaceToken(tokens, Token::Type::COLON, scanState); break;
        case ';': emplaceToken(tokens, Token::Type::SEMICOLON, scanState); break;
        case '~': emplaceToken(tokens, Token::Type::TILDA, scanState); break;
        case '?': emplaceToken(tokens, Token::Type::QUESTION, scanState); break;

        // Operators
        case '!': emplaceToken(tokens, scanState.match('=') ? Token::Type::NOT_EQUAL : Token::Type::NOT, scanState); break;
        case '=': emplaceToken(tokens, scanState.match('=') ? Token::Type::EQUAL_EQUAL : Token::Type::EQUAL, scanState); break;
        case '<': emplaceToken(tokens, scanState.match('=') ? Token::Type::LESS_EQUAL : Token::Type::LESS, scanState); break;
        case '>': emplaceToken(tokens, scanState.match('=') ? Token::Type::GREATER_EQUAL : Token::Type::GREATER, scanState); break;

        // Assignment operators
        case '*': emplaceToken(tokens, scanState.match('=') ? Token::Type::STAR_EQUAL : Token::Type::STAR, scanState); break;
        //case '/': emplaceToken(tokens, scanState.match('=') ? Token::Type::SLASH_EQUAL : Token::Type::SLASH, scanState); break;
        case '%': emplaceToken(tokens, scanState.match('=') ? Token::Type::PERCENT_EQUAL : Token::Type::PERCENT, scanState); break;
        case '+': emplaceToken(tokens, scanState.match('=') ? Token::Type::PLUS_EQUAL : Token::Type::PLUS, scanState); break;
        case '-': emplaceToken(tokens, scanState.match('=') ? Token::Type::MINUS_EQUAL : Token::Type::MINUS, scanState); break;        
        case '^': emplaceToken(tokens, scanState.match('=') ? Token::Type::CARET_EQUAL : Token::Type::CARET, scanState); break;
        
        case '&': 
        {
            if(scanState.match('=')) {
                emplaceToken(tokens, Token::Type::AMPERSAND_EQUAL, scanState);
            }
            else if(scanState.match('&')) {
                emplaceToken(tokens, Token::Type::AMPERSAND_AMPERSAND, scanState);
            }
            else {
                emplaceToken(tokens, Token::Type::AMPERSAND, scanState);
            }
            break;
        }

        case '|': 
        {
            if(scanState.match('=')) {
                emplaceToken(tokens, Token::Type::PIPE_EQUAL, scanState);
            }
            else if(scanState.match('|')) {
                emplaceToken(tokens, Token::Type::PIPE_PIPE, scanState);
            }
            else {
                emplaceToken(tokens, Token::Type::PIPE, scanState);
            }
            break;
        }
        
        case '/':
        {
            // Line comment
            if(scanState.match('/')) {
                while(scanState.peek() != '\n' && !scanState.isAtEnd()) {
                    scanState.advance();
                }
            }
            else {
                emplaceToken(tokens, Token::Type::SLASH, scanState);
            }
            break;
        }

        // Whitespace
        case ' ':
        case '\r':
        case '\t':
            break;

        // New line
        case '\n': scanState.nextLine(); break;

        default:
        {
            // If we have a digit or a period, scan number
            if(std::isdigit(c) || c == '.') {
                scanNumber(c, scanState, tokens);
            }
            // Otherwise, scan identifier
            else if(std::isalpha(c) || c == '_') {
                scanIdentifier(scanState, tokens);
            }
            else {
                scanState.error("Unexpected character.");
            }
        }
    }
}
}

//---------------------------------------------------------------------------
// MiniParse::Scanner
//---------------------------------------------------------------------------
namespace MiniParse::Scanner
{
std::vector<Token> scanSource(const std::string_view &source, ErrorHandler &errorHandler)
{
    std::vector<Token> tokens;

    ScanState scanState(source, errorHandler);

    // Current line
    size_t line = 1;

    // Scan tokens
    while(!scanState.isAtEnd()) {
        scanState.resetLexeme();
        scanToken(scanState, tokens);
    }

    emplaceToken(tokens, Token::Type::END_OF_FILE, scanState);
    return tokens;
}
}