#pragma once

// Standard C++ includes
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Mini-parse includes
#include "token.h"

//---------------------------------------------------------------------------
// Parser::Scanner::Cursor
//---------------------------------------------------------------------------
namespace Parser::Scanner
{

//---------------------------------------------------------------------------
// Parser::Scanner::Error
//---------------------------------------------------------------------------
class Error : public std::runtime_error
{
public:
    Error(size_t line, const std::string &message)
        : line(line), std::runtime_error(message)
    {}

    const size_t line;
};

//---------------------------------------------------------------------------
// Parser::Scanner::ScannerErrorUnsupported
//---------------------------------------------------------------------------
class ErrorUnsupported : public Error
{
public:
    ErrorUnsupported(size_t line, const std::string &message)
        : Error(line, message)
    {}
};

std::vector<Token> scanTokens(const std::string_view &source);

}   // namespace Scanner