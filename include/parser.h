#pragma once

// Standard C++ includes
#include <memory>
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "token.h"

// Forward declarations
namespace MiniParse
{
class ErrorHandler;
}

//---------------------------------------------------------------------------
// MiniParse::Scanner::Parser
//---------------------------------------------------------------------------
namespace MiniParse::Parser
{
std::unique_ptr<const Expression::Base> parseTokens(const std::vector<Token> &tokens, ErrorHandler &errorHandler);
}   // MiniParse::MiniParse