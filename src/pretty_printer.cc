#include "pretty_printer.h"

// Mini-parse includes
#include "type.h"
#include "utils.h"

//---------------------------------------------------------------------------
// MiniParse::PrettyPrinter
//---------------------------------------------------------------------------
namespace MiniParse
{
std::string PrettyPrinter::print(const Statement::StatementList &statements)
{
    // Clear string stream
    m_StringStream.str("");

    for(auto &s : statements) {
        s.get()->accept(*this);
        m_StringStream << std::endl;
    }
    
    // Return string stream contents
    return m_StringStream.str();
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::ArraySubscript &arraySubscript)
{
    m_StringStream << arraySubscript.getArrayName().lexeme << "[";
    arraySubscript.getIndex()->accept(*this);
    m_StringStream << "]";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Assignment &assignement)
{
    m_StringStream << assignement.getVarName().lexeme << " " << assignement.getOperator().lexeme << " ";
    assignement.getValue()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Binary &binary)
{
    binary.getLeft()->accept(*this);
    m_StringStream << " " << binary.getOperator().lexeme << " ";
    binary.getRight()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Call &call)
{
    call.getCallee()->accept(*this);
    m_StringStream << "(";
    for(const auto &a : call.getArguments()) {
        a->accept(*this);
    }
    m_StringStream << ")";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Cast &cast)
{
    m_StringStream << "(" << cast.getType()->getTypeName() << ")";
    cast.getExpression()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Conditional &conditional)
{
    conditional.getCondition()->accept(*this);
    m_StringStream << " ? ";
    conditional.getTrue()->accept(*this);
    m_StringStream << " : ";
    conditional.getFalse()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Grouping &grouping)
{
    m_StringStream << "(";
    grouping.getExpression()->accept(*this);
    m_StringStream << ")";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Literal &literal)
{
    std::visit(
        Utils::Overload{
            [this](auto x) { m_StringStream << x; },
            [this](std::monostate) { m_StringStream << "invalid"; }},
        literal.getValue());
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Logical &logical)
{
    logical.getLeft()->accept(*this);
    m_StringStream << " " << logical.getOperator().lexeme << " ";
    logical.getRight()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::PostfixIncDec &postfixIncDec)
{
    m_StringStream << postfixIncDec.getVarName().lexeme << postfixIncDec.getOperator().lexeme;
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::PrefixIncDec &prefixIncDec)
{
    m_StringStream << prefixIncDec.getOperator().lexeme << prefixIncDec.getVarName().lexeme;
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Variable &variable)
{
    m_StringStream << variable.getName().lexeme;
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Expression::Unary &unary)
{
    m_StringStream << unary.getOperator().lexeme;
    unary.getRight()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Break&)
{
    m_StringStream << "break;";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Compound &compound)
{
    m_StringStream << "{" << std::endl;
    for(auto &s : compound.getStatements()) {
        s->accept(*this);
        m_StringStream << std::endl;
    }
    m_StringStream << "}" << std::endl;
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Continue&)
{
    m_StringStream << "continue;";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Do &doStatement)
{
    m_StringStream << "do";
    doStatement.getBody()->accept(*this);
    m_StringStream << "while(";
    doStatement.getCondition()->accept(*this);
    m_StringStream << ");" << std::endl;
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Expression &expression)
{
    expression.getExpression()->accept(*this);
    m_StringStream << ";";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::For &forStatement)
{
    m_StringStream << "for(";
    if(forStatement.getInitialiser()) {
        forStatement.getInitialiser()->accept(*this);
    }
    else {
        m_StringStream << ";";
    }
    m_StringStream << " ";

    if(forStatement.getCondition()) {
        forStatement.getCondition()->accept(*this);
    }

    m_StringStream << "; ";
    if(forStatement.getIncrement()) {
        forStatement.getIncrement()->accept(*this);
    }
    m_StringStream << ")";
    forStatement.getBody()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::If &ifStatement)
{
    m_StringStream << "if(";
    ifStatement.getCondition()->accept(*this);
    m_StringStream << ")" << std::endl;
    ifStatement.getThenBranch()->accept(*this);
    if(ifStatement.getElseBranch()) {
        m_StringStream << "else" << std::endl;
        ifStatement.getElseBranch()->accept(*this);
    }
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Labelled &labelled)
{
    m_StringStream << labelled.getKeyword().lexeme << " ";
    if(labelled.getValue()) {
        labelled.getValue()->accept(*this);
    }
    m_StringStream << " : ";
    labelled.getBody()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Switch &switchStatement)
{
    m_StringStream << "switch(";
    switchStatement.getCondition()->accept(*this);
    m_StringStream << ")" << std::endl;
    switchStatement.getBody()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::VarDeclaration &varDeclaration)
{
    if(varDeclaration.isConst()) {
        m_StringStream << "const ";
    }
    m_StringStream << varDeclaration.getType()->getTypeName() << " ";

    for(const auto &var : varDeclaration.getInitDeclaratorList()) {
        m_StringStream << std::get<0>(var).lexeme;
        if(std::get<1>(var) != nullptr) {
            m_StringStream << " = ";
            std::get<1>(var)->accept(*this);
        }
        m_StringStream << ", ";
    }
    m_StringStream << ";";
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::While &whileStatement)
{
    m_StringStream << "while(";
    whileStatement.getCondition()->accept(*this);
    m_StringStream << ")" << std::endl;
    whileStatement.getBody()->accept(*this);
}
//---------------------------------------------------------------------------
void PrettyPrinter::visit(const Statement::Print &print)
{
    m_StringStream << "print ";
    print.getExpression()->accept(*this);
    m_StringStream << ";";
}
}
