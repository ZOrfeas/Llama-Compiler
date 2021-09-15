#include <iostream>
#include <iomanip>

#include "ast.hpp"
#include "parser.hpp"

const std::string type_string[] = {"unit", "int", "float", "bool", "char"};

// Stores the amount of spaces to be inserted before printing the node
static int prefixSpaces = 0;
const int indent = 2;

// Prints newline
void createBlock(std::ostream &out)
{
    prefixSpaces += indent;
    out << std::endl;
}

// Doesn't print newline
void closeBlock(std::ostream &out)
{
    prefixSpaces -= indent;

    //out << std::endl;
}

void printHeader(std::ostream &out, std::string h)
{
    out << std::string(prefixSpaces, ' ') << '-' << h;
}
void printType(std::ostream &out, std::string t);

std::ostream &operator<<(std::ostream &out, const AST &t)
{
    t.printOn(out);
    return out;
}

std::string UnknownType::getTypeStr() const
{
    return TG->stringifyTypeClean();
}
std::string BasicType::getTypeStr() const
{
    return type_string[static_cast<int>(t)];
}
std::string FunctionType::getTypeStr() const
{
    return lhtype->getTypeStr() + "->" + rhtype->getTypeStr();
}
std::string ArrayType::getTypeStr() const
{
    // 2d array of int
    return std::to_string(dimensions) + "d " + "array of " + elem_type->getTypeStr();
}
std::string RefType::getTypeStr() const
{
    // int ref
    return ref_type->getTypeStr() + " ref";
}
std::string CustomType::getTypeStr() const
{
    return id;
}

void Type::printOn(std::ostream &out) const
{
    printHeader(out, "Type " + getTypeStr());

    createBlock(out);
    closeBlock(out);
}

/*
void UnknownType::printOn(std::ostream &out) const
{
    printHeader(out, "UnknownType " + getTypeStr());
}
void BasicType::printOn(std::ostream &out) const
{
    printHeader(out, "BasicType " + getTypeStr());
}
void FunctionType::printOn(std::ostream &out) const
{
    printHeader(out, "FunctionType");
    out << *lhtype << "->" << *rhtype;
}
void ArrayType::printOn(std::ostream &out) const
{
    printHeader(out, "ArrayType " + std::to_string(dimensions) + " ");
    out << *elem_type;
}
void RefType::printOn(std::ostream &out) const
{
    printHeader(out, "RefType ");
    out << *ref_type;
}
void CustomType::printOn(std::ostream &out) const
{
    printHeader(out, "CustomType " + getTypeStr());
}
*/

void Constr::printOn(std::ostream &out) const
{
    printHeader(out, "Constr " + Id);

    createBlock(out);
    for (auto *t : type_list)
    {
        out << *t;
    }
    closeBlock(out);
}
void Par::printOn(std::ostream &out) const
{
    printHeader(out, "Par " + id);

    createBlock(out);
    out << *T;
    closeBlock(out);
}

void Tdef::printOn(std::ostream &out) const
{
    printHeader(out, "Tdef " + id);

    createBlock(out);
    for (Constr *c : constr_list)
    {
        out << *c;
    }
    closeBlock(out);
}
void Constant::printOn(std::ostream &out) const
{
    printHeader(out, "Constant " + id);

    createBlock(out);
    out << *T;
    out << *expr;
    closeBlock(out);
}
void Function::printOn(std::ostream &out) const
{
    printHeader(out, "Function " + id);

    createBlock(out);
    out << *T;

    printHeader(out, "Parameters");
    createBlock(out);
    for (Par *p : par_list)
    {
        out << *p;
    }
    closeBlock(out);

    printHeader(out, "Body");
    createBlock(out);
    out << *expr;
    closeBlock(out);

    closeBlock(out);
}
void Array::printOn(std::ostream &out) const
{
    printHeader(out, "Array " + id);

    createBlock(out);
    out << *T;

    for (auto *e : expr_list)
    {
        out << *e;
    }
    closeBlock(out);
}
void Variable::printOn(std::ostream &out) const
{
    printHeader(out, "Variable " + id);

    createBlock(out);
    out << *T;
    closeBlock(out);
}

void Letdef::printOn(std::ostream &out) const
{
    if (recursive)
        printHeader(out, "Letdef rec");
    else
        printHeader(out, "Letdef");

    createBlock(out);
    for (auto *d : def_list)
    {
        out << *d;
    }
    closeBlock(out);
}
void Typedef::printOn(std::ostream &out) const
{
    printHeader(out, "Typedef");

    createBlock(out);
    for (auto *t : tdef_list)
    {
        out << *t;
    }
    closeBlock(out);
}
void Program::printOn(std::ostream &out) const
{
    printHeader(out, "Program");

    createBlock(out);
    for (auto *d : definition_list)
    {
        out << *d;
    }
    closeBlock(out);
}

void LetIn::printOn(std::ostream &out) const
{
    printHeader(out, "LetIn");

    createBlock(out);
    out << *letdef;
    out << *expr;
    closeBlock(out);
}

void String_literal::printOn(std::ostream &out) const
{
    printHeader(out, "String_literal " + originalStr);

    createBlock(out);
    closeBlock(out);
}
void Char_literal::printOn(std::ostream &out) const
{
    printHeader(out, "Char_literal " + c_string);

    createBlock(out);
    closeBlock(out);
}
void Bool_literal::printOn(std::ostream &out) const
{
    printHeader(out, "Bool_literal " + std::to_string(b));

    createBlock(out);
    closeBlock(out);
}
void Float_literal::printOn(std::ostream &out) const
{
    printHeader(out, "Float_literal " + std::to_string(d));

    createBlock(out);
    closeBlock(out);
}
void Int_literal::printOn(std::ostream &out) const
{
    printHeader(out, "Int_literal " + std::to_string(n));

    createBlock(out);
    closeBlock(out);
}
void Unit_literal::printOn(std::ostream &out) const
{
    printHeader(out, "Unit_literal");

    createBlock(out);
    closeBlock(out);
}

std::string opToString(int op)
{
    switch (op)
    {
        case '!':
        case '+':
        case '-': 
        case '*':
        case '/':
        case '=':
        case '<':
        case '>':
        case ';':           return std::string(1, char(op));
        case T_mod:         return "mod";
        case T_plusdot:     return "+.";
        case T_minusdot:    return "-.";
        case T_stardot:     return "*.";
        case T_slashdot:    return "/.";
        case T_dblstar:     return "**";
        case T_dblbar:      return "||";
        case T_dblampersand:return "&&";
        case T_lessgreater: return "<>";
        case T_dbleq:       return "==";
        case T_exclameq:    return "!=";
        case T_leq:         return "<=";
        case T_geq:         return ">=";
        case T_coloneq:     return ":=";
        case T_not:         return "not";
        case T_delete:      return "delete";
        default:            return ""; break;
    }
}
void BinOp::printOn(std::ostream &out) const
{
    std::string opStr = opToString(op);
    if(opStr == "") printHeader(out, "BinOp " + std::to_string(op));
    else printHeader(out, "BinOp " + opStr);

    createBlock(out);
    out << *lhs;
    out << *rhs;
    closeBlock(out);
}
void UnOp::printOn(std::ostream &out) const
{
    std::string opStr = opToString(op);
    if(opStr == "") printHeader(out, "UnOp " + std::to_string(op));
    else printHeader(out, "UnOp " + opStr);

    createBlock(out);
    out << *expr; 
    closeBlock(out);
}
void New::printOn(std::ostream &out) const
{
    printHeader(out, "New");

    createBlock(out);
    out << *new_type;
    closeBlock(out);
}

void While::printOn(std::ostream &out) const
{
    printHeader(out, "While");

    createBlock(out);
    out << *cond;
    out << *body;
    closeBlock(out);
}
void For::printOn(std::ostream &out) const
{
    printHeader(out, "For " + id);

    createBlock(out);
    out << *start;
    printHeader(out, step);
    out << *finish;
    out << *body;
    closeBlock(out);
}
void If::printOn(std::ostream &out) const
{
    printHeader(out, "If");

    createBlock(out);
    out << *cond;
    out << *body;
    if (else_body)
        out << *else_body;
    closeBlock(out);
}

void Dim::printOn(std::ostream &out) const
{
    printHeader(out, "Dim");

    createBlock(out);
    if (dim)
        out << *dim;
    out << id;
    closeBlock(out);
}

void ConstantCall::printOn(std::ostream &out) const
{
    printHeader(out, "ConstantCal " + id);

    createBlock(out);
    closeBlock(out);
}
void FunctionCall::printOn(std::ostream &out) const
{
    printHeader(out, "FunctionCall " + id);

    createBlock(out);
    for (auto *e : expr_list)
    {
        out << *e;
    }
    closeBlock(out);
}
void ConstructorCall::printOn(std::ostream &out) const
{
    printHeader(out, "ConstructorCall " + Id);

    createBlock(out);
    for (auto *e : expr_list)
    {
        out << *e;
    }
    closeBlock(out);
}
void ArrayAccess::printOn(std::ostream &out) const
{
    printHeader(out, "ArrayAccess " + id);

    createBlock(out);
    for (auto *e : expr_list)
    {
        out << *e;
    }
    closeBlock(out);
}

void PatternLiteral::printOn(std::ostream &out) const
{
    printHeader(out, "PatternLiteral");

    createBlock(out);
    out << *literal;
    closeBlock(out);
}
void PatternId::printOn(std::ostream &out) const
{
    printHeader(out, "PatternId " + id);

    createBlock(out);
    closeBlock(out);
}
void PatternConstr::printOn(std::ostream &out) const
{
    printHeader(out, "PatternConstr " + Id);

    createBlock(out);
    for (Pattern *p : pattern_list)
    {
        out << *p;
    }
    closeBlock(out);
}

void Clause::printOn(std::ostream &out) const
{
    printHeader(out, "Clause");

    createBlock(out);

    printHeader(out, "Pattern");
    createBlock(out);
    out << *pattern;
    closeBlock(out);

    printHeader(out, "Expression");
    createBlock(out);
    out << *expr;
    closeBlock(out);

    closeBlock(out);
}
void Match::printOn(std::ostream &out) const
{
    printHeader(out, "Match");

    createBlock(out);
    for (Clause *c : clause_list)
    {
        out << *c;
    }
    closeBlock(out);
}
