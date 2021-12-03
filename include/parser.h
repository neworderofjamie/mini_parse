#pragma once

// Standard C++ includes
#include <memory>
#include <vector>

// Mini-parse includes
#include "expression.h"
#include "statement.h"
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
std::unique_ptr<const Expression::Base> parseExpression(const std::vector<Token> &tokens, ErrorHandler &errorHandler);

std::vector<std::unique_ptr<const Statement::Base>> parseStatements(const std::vector<Token> &tokens, ErrorHandler &errorHandler);
}   // MiniParse::MiniParse