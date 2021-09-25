#include <string>
#include <cstdio>
#include <cstdlib>

#include "lexer.hpp"
#include "ast.hpp"

std::vector<Identifier *> AST_identifier_list = {};

void printColorString(std::string s, int width, int format = 0, int color = 37)
{
    std::string intro = (format == 0) ? "\033[0m"
                                      : "\033[" + std::to_string(format) + ";" + std::to_string(color) + "m";
    std::string outro = "\033[0m";

    std::cout << intro
              << std::left << std::setfill(' ') << std::setw(width)
              << s
              << outro;
}
Identifier::Identifier(std::string id, int line)
    : id(id), line(line)
{
    TG = st.lookup(id)->getTypeGraph();
}
std::string Identifier::getName()
{
    return id;
}
std::string Identifier::getTypeString()
{
    TypeGraph *correctTypeGraph = inf.deepSubstitute(TG);
    return correctTypeGraph->stringifyTypeClean();
}
std::string Identifier::getLine()
{
    return std::to_string(line);
}
void Identifier::printIdLine(int lineWidth, int idWidth, int typeWidth)
{
    printColorString(getName(), idWidth, 1, 35);
    printColorString(getTypeString(), typeWidth);
    printColorString(getLine(), lineWidth);
    std::cout << std::endl;
}

void AST::printError(std::string msg, bool crash)
{
    std::string intro = "Error at line " + std::to_string(line_number) + ": ";
    printColorString(intro, intro.size(), 1, 31);
    std::cout << msg << std::endl;
    if (crash) exit(1);
}
void AST::printIdTypeGraphs()
{
    // Find the maximum size of the ids, type and line strings
    // in order to decide the padding
    int margin = 2;
    int idWidth = 4, typeWidth = 8, lineWidth = 3;
    std::string name, typeString, line;
    for (auto ident : AST_identifier_list)
    {
        name = ident->getName();
        typeString = ident->getTypeString();
        line = ident->getLine();

        if (idWidth < (int)name.size())
            idWidth = name.size();
        if (typeWidth < (int)typeString.size())
            typeWidth = typeString.size();
        if (lineWidth < (int)line.size())
            lineWidth = line.size();
    }
    idWidth += margin;
    typeWidth += margin;
    lineWidth += margin;

    // Print the header of the table
    int colorANSI = 34, formatANSI = 4;
    printColorString("Name", idWidth, formatANSI, colorANSI);
    printColorString("Type", typeWidth, formatANSI, colorANSI);
    printColorString("Line", lineWidth, formatANSI, colorANSI);
    std::cout << std::endl;

    // Print every line
    for (auto ident : AST_identifier_list)
    {
        ident->printIdLine(lineWidth, idWidth, typeWidth);
    }
}
void AST::addToIdList(std::string id)
{
    AST_identifier_list.push_back(new Identifier(id, line_number));
}
AST::AST()
{
    line_number = yylineno;
}
AST::~AST()
{
}
Type::Type(category c)
    : c(c)
{
}
UnknownType::UnknownType()
    : Type(category::CATEGORY_unknown)
{
    TG = new UnknownTypeGraph(true, true, false);
}
BasicType::BasicType(type t)
    : Type(category::CATEGORY_basic), t(t)
{
}
FunctionType::FunctionType(Type *lhtype, Type *rhtype)
    : Type(category::CATEGORY_function), lhtype(lhtype), rhtype(rhtype)
{
}
ArrayType::ArrayType(int dimensions, Type *elem_type)
    : Type(category::CATEGORY_array), dimensions(dimensions), elem_type(elem_type)
{
}
RefType::RefType(Type *ref_type)
    : Type(category::CATEGORY_ref), ref_type(ref_type)
{
}
CustomType::CustomType(std::string *id)
    : Type(category::CATEGORY_custom), id(*id)
{
}

Constr::Constr(std::string *Id, std::vector<Type *> *t)
    : Id(*Id), type_list(*t) {}
Par::Par(std::string *id, Type *t)
    : id(*id), T(t) {}

DefStmt::DefStmt(std::string id)
    : id(id) {}
Tdef::Tdef(std::string *id, std::vector<Constr *> *c)
    : DefStmt(*id), constr_list(*c) {}
Def::Def(std::string id, Type *t)
    : DefStmt(id), T(t) {}
Constant::Constant(std::string *id, Expr *e, Type *t)
    : Def(*id, t), expr(e) {}
Function::Function(std::string *id, std::vector<Par *> *p, Expr *e, Type *t)
    : Constant(id, e, t), par_list(*p), 
    funcPrototype(nullptr), envStructType(nullptr) {}
Mutable::Mutable(std::string id, Type *T)
    : Def(id, T) {}
Array::Array(std::string *id, std::vector<Expr *> *e, Type *T)
    : Mutable(*id, T), expr_list(*e) {}
Variable::Variable(std::string *id, Type *T)
    : Mutable(*id, T) {}

Letdef::Letdef(std::vector<DefStmt *> *d, bool rec)
    : recursive(rec), def_list(*d) {}
Typedef::Typedef(std::vector<DefStmt *> *t)
    : tdef_list(*t) {}
Program::~Program()
{
}
Program::Program()
    : definition_list() {}
void Program::append(Definition *d)
{
    definition_list.push_back(d);
}

LetIn::LetIn(Definition *letdef, Expr *expr)
    : letdef(letdef), expr(expr) {}

char Literal::getChar(std::string c)
{
    char ans = 0;

    // Normal character
    if (c[0] != '\\')
    {
        ans = c[0];
    }

    // '\xnn'
    else if (c[1] == 'x')
    {
        const char hex[2] = {c[2], c[3]};
        ans = strtol(hex, nullptr, 16);
    }

    // Escape secuence
    else
    {
        switch (c[1])
        {
        case 'n':
            ans = '\n';
            break;
        case 't':
            ans = '\t';
            break;
        case 'r':
            ans = '\r';
            break;
        case '0':
            ans = 0;
            break;
        case '\\':
            ans = '\\';
            break;
        case '\'':
            ans = '\'';
            break;
        case '\"':
            ans = '\"';
            break;
        }
    }

    return ans;
}
String_literal::String_literal(std::string *s)
    : s(escapeChars(s->substr(1, s->size() - 2))), originalStr(*s) {}
Char_literal::Char_literal(std::string *c_string)
    : c_string(c_string->substr(1, c_string->size() - 2))
{
    c = getChar(this->c_string);
}
Bool_literal::Bool_literal(bool b)
    : b(b) {}
Float_literal::Float_literal(double d)
    : d(d) {}
Int_literal::Int_literal(int n)
    : n(n) {}
Unit_literal::Unit_literal()
{
}

BinOp::BinOp(Expr *e1, int op, Expr *e2)
    : lhs(e1), rhs(e2), op(op) {}
UnOp::UnOp(int op, Expr *e)
    : expr(e), op(op) {}
New::New(Type *t)
    : new_type(t) {}

While::While(Expr *e1, Expr *e2)
    : cond(e1), body(e2) {}
For::For(std::string *id, Expr *e1, std::string s, Expr *e2, Expr *e3)
    : id(*id), step(s), start(e1), finish(e2), body(e3) {}
If::If(Expr *e1, Expr *e2, Expr *e3)
    : cond(e1), body(e2), else_body(e3) {}

Dim::Dim(std::string *id, Int_literal *dim)
    : dim(dim), id(*id) {}

ConstantCall::ConstantCall(std::string *id)
    : id(*id) {}
FunctionCall::FunctionCall(std::string *id, std::vector<Expr *> *expr_list)
    : ConstantCall(id), expr_list(*expr_list) {}
ConstructorCall::ConstructorCall(std::string *Id, std::vector<Expr *> *expr_list)
    : Id(*Id), expr_list(*expr_list) {}
ArrayAccess::ArrayAccess(std::string *id, std::vector<Expr *> *expr_list)
    : id(*id), expr_list(*expr_list) {}

PatternLiteral::PatternLiteral(Literal *l)
    : literal(l) {}
PatternId::PatternId(std::string *id)
    : id(*id) {}
PatternConstr::PatternConstr(std::string *Id, std::vector<Pattern *> *p_list)
    : Id(*Id), pattern_list(*p_list)
{
    constrTypeGraph = nullptr;
}

Clause::Clause(Pattern *p, Expr *e)
    : pattern(p), expr(e) {}
Match::Match(Expr *e, std::vector<Clause *> *c)
    : toMatch(e), clause_list(*c) {}
