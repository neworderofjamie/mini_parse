// Standard C++ includes
#include <iostream>
#include <regex>
#include <string>
#include <variant>

// Standard C includes
#include <cassert>
#include <cmath>

// Mini-parse includes
#include "error_handler.h"
#include "expression.h"
#include "interpreter.h"
#include "parser.h"
#include "pretty_printer.h"
#include "scanner.h"
#include "type.h"
#include "type_checker.h"
#include "utils.h"

using namespace MiniParse;

std::string test(
    "if($(outRow) == $(maxOutRow)) {\n"
    "   $(endRow);\n"
    "}\n"
    "const int strideRow = ($(outRow) * (int)$(conv_sh)) - (int)$(conv_padh);\n"
    "const int kernRow = $(inRow) - strideRow;\n"
    "for(int outCol = $(minOutCol); outCol < $(maxOutCol); outCol++) {\n"
    "    const int strideCol = (outCol * (int)$(conv_sw)) - (int)$(conv_padw);\n"
    "    const int kernCol = $(inCol) - strideCol;\n"
    "    for(unsigned int outChan = 0; outChan < (unsigned int)$(conv_oc); outChan++) {\n"
    "        const int idPost = (($(outRow) * (int)$(conv_ow) * (int)$(conv_oc)) +\n"
    "                            (outCol * (int)$(conv_oc)) +\n"
    "                            outChan);\n"
    "        $(addSynapse, idPost, kernRow, kernCol, $(inChan), outChan);\n"
    "    }\n"
    "}\n"
    "$(outRow)++;\n");

std::string test2(
    "if ($(RefracTime) <= 0.0) {\n"
    "  double alpha = (($(Isyn) + $(Ioffset)) * $(Rmembrane)) + $(Vrest);\n"
    "  $(V) = alpha - ($(ExpTC) * (alpha - $(V)));\n"
    "}\n"
    "else {\n"
    "  $(RefracTime) -= DT;\n"
    "}\n");

std::string test3(
    "double Imem;\n"
    "unsigned int mt;\n"
    "double mdt= DT/25.0;\n"
    "for (mt=0; mt < 25; mt++) {\n"
    "   Imem= -($(m)*$(m)*$(m)*$(h)*$(gNa)*($(V)-($(ENa)))+\n"
    "       $(n)*$(n)*$(n)*$(n)*$(gK)*($(V)-($(EK)))+\n"
    "       $(gl)*($(V)-($(El)))-$(Isyn));\n"
    "   double _a;\n"
    "   if ($(V) == -52.0) {\n"
    "       _a= 1.28;\n"
    "   }\n"
    "   else {\n"
    "       _a= 0.32*(-52.0-$(V))/(exp((-52.0-$(V))/4.0)-1.0);\n"
    "   }\n"
    "   double _b;\n"
    "   if ($(V) == -25.0) {\n"
    "       _b= 1.4;\n"
    "   }\n"
    "   else {\n"
    "       _b= 0.28*($(V)+25.0)/(exp(($(V)+25.0)/5.0)-1.0);\n"
    "   }\n"
    "   $(m)+= (_a*(1.0-$(m))-_b*$(m))*mdt;\n"
    "   _a= 0.128*exp((-48.0-$(V))/18.0);\n"
    "   _b= 4.0 / (exp((-25.0-$(V))/5.0)+1.0);\n"
    "   $(h)+= (_a*(1.0-$(h))-_b*$(h))*mdt;\n"
    "   if ($(V) == -50.0) {\n"
    "       _a= 0.16;\n"
    "   }\n"
    "   else {\n"
    "       _a= 0.032*(-50.0-$(V))/(exp((-50.0-$(V))/5.0)-1.0);\n"
    "   }\n"
    "   _b= 0.5*exp((-55.0-$(V))/40.0);\n"
    "   $(n)+= (_a*(1.0-$(n))-_b*$(n))*mdt;\n"
    "   $(V)+= Imem/$(C)*mdt;\n"
    "}\n");

std::string removeOldStyleVar(const std::string &input) {
    std::regex variable(R"(\$\(([_a-zA-Z][a-zA-Z0-9]*)\))");
 
    return std::regex_replace(input, variable, "$1");
}

class ErrorHandler : public MiniParse::ErrorHandler
{
public:
    ErrorHandler() : m_Error(false)
    {}

    bool hasError() const { return m_Error; }

    virtual void error(size_t line, std::string_view message) override
    {
        report(line, "", message);
    }

    virtual void error(const Token &token, std::string_view message) override
    {
        if(token.type == MiniParse::Token::Type::END_OF_FILE) {
            report(token.line, " at end", message);
        }
        else {
            report(token.line, " at '" + std::string{token.lexeme} + "'", message);
        }
    }

private:
    void report(size_t line, std::string_view where, std::string_view message)
    {
        std::cerr << "[line " << line << "] Error" << where << ": " << message << std::endl;
        m_Error = true;
    }

    bool m_Error;
};

class Sqrt : public Interpreter::Callable
{
    using LiteralValue = Token::LiteralValue;
public:
    virtual std::optional<size_t> getArity() const final
    {
        return 1;
    }

    virtual LiteralValue call(const std::vector<LiteralValue> &arguments) final
    {
       return std::visit(
            MiniParse::Utils::Overload{
                [](auto v) { return LiteralValue{std::sqrt(v)}; },
                [](std::monostate) { return LiteralValue(); }},
            arguments[0]);
    }
};
    

int main()
{
    ::ErrorHandler errorHandler;
    try
    {
        // Scan
        /*const std::string source = removeOldStyleVar(test3);
        const auto tokens = MiniParse::Scanner::scanSource(
            source, errorHandler);
        const auto tokens = MiniParse::Scanner::scanSource(
            "int x = 4, y;\n"
            "print ((12 + x) * 5) + 3;\n"
            "y = 12;\n"
            "print y > 4;\n"
            "print y;\n"
            "y *= 2;\n"
            "print y;\n"
            "print 100;\n"
            "print true;\n", errorHandler);
        const auto tokens = MiniParse::Scanner::scanSource(
            "int x = 4;\n"
            "print x;\n"
            "{\n"
            "    int x = 7, y = 8;\n"
            "    print x;\n"
            "}\n"
            "print x;\n"
            "print y;\n", errorHandler);
        const auto tokens = MiniParse::Scanner::scanSource(
            "double x = 2.0f;\n"
            "print x;\n"
            "print sqrt(x);\n", errorHandler);*/
        const auto tokens = Scanner::scanSource(
            "float x = 1.0f;"
            "while(true) {\n"
            "   x <<= 1;\n"
            "   print x;\n"
            "   if(x < 0.1f) {\n"
            "       break;\n"
            "   }\n"
            "}\n", errorHandler);
        // Parse
        auto statements = Parser::parseStatements(tokens, errorHandler);
        
        TypeChecker::Environment typeEnvironment;
        
        typeEnvironment.define<Type::Double>("Isyn", true); 
        typeEnvironment.define<Type::Double>("gNa", true);
        typeEnvironment.define<Type::Double>("ENa", true);
        typeEnvironment.define<Type::Double>("gK", true);
        typeEnvironment.define<Type::Double>("EK", true);
        typeEnvironment.define<Type::Double>("gl", true);
        typeEnvironment.define<Type::Double>("El", true);
        typeEnvironment.define<Type::Double>("C", true);
        typeEnvironment.define<Type::Double>("V");
        typeEnvironment.define<Type::Double>("m");
        typeEnvironment.define<Type::Double>("h");
        typeEnvironment.define<Type::Double>("n");
        typeEnvironment.define<Type::Exp>("exp");
        typeEnvironment.define<Type::Sqrt>("sqrt");
        TypeChecker::typeCheck(statements, typeEnvironment, errorHandler);
       
        PrettyPrinter printer;
        std::cout << printer.print(statements) << std::endl;
        

        Sqrt sqrt;
        Interpreter::Environment environment;
        environment.define("sqrt", sqrt);
        Interpreter interpreter;
        interpreter.interpret(statements, environment);
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}