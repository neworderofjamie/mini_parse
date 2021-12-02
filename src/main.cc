// Standard C++ includes
#include <iostream>
#include <regex>
#include <string>

#include "expression.h"
#include "scanner.h"

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
    "  scalar alpha = (($(Isyn) + $(Ioffset)) * $(Rmembrane)) + $(Vrest);\n"
    "  $(V) = alpha - ($(ExpTC) * (alpha - $(V)));\n"
    "}\n"
    "else {\n"
    "  $(RefracTime) -= DT;\n"
    "}\n");

std::string test3(
    "scalar Imem;\n"
    "unsigned int mt;\n"
    "scalar mdt= DT/25.0;\n"
    "for (mt=0; mt < 25; mt++) {\n"
    "   Imem= -($(m)*$(m)*$(m)*$(h)*$(gNa)*($(V)-($(ENa)))+\n"
    "       $(n)*$(n)*$(n)*$(n)*$(gK)*($(V)-($(EK)))+\n"
    "       $(gl)*($(V)-($(El)))-$(Isyn));\n"
    "   scalar _a;\n"
    "   if (lV == -52.0) {\n"
    "       _a= 1.28;\n"
    "   }\n"
    "   else {\n"
    "       _a= 0.32*(-52.0-$(V))/(exp((-52.0-$(V))/4.0)-1.0);\n"
    "   }\n"
    "   scalar _b;\n"
    "   if (lV == -25.0) {\n"
    "       _b= 1.4;\n"
    "   }\n"
    "   else {\n"
    "       _b= 0.28*($(V)+25.0)/(exp(($(V)+25.0)/5.0)-1.0);\n"
    "   }\n"
    "   $(m)+= (_a*(1.0-$(m))-_b*$(m))*mdt;\n"
    "   _a= 0.128*exp((-48.0-$(V))/18.0);\n"
    "   _b= 4.0 / (exp((-25.0-$(V))/5.0)+1.0);\n"
    "   $(h)+= (_a*(1.0-$(h))-_b*$(h))*mdt;\n"
    "   if (lV == -50.0) {\n"
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
int main()
{
    using namespace Parser;

    const auto test2Fix = removeOldStyleVar(test2);
    try
    {
        //const auto tokens = Scanner::scanTokens(test2Fix);

       

    }
    catch(const Scanner::Error &s) {
        std::cerr << s.what() << std::endl;
    }
    catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}