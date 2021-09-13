#include <iostream>
#include <iomanip>

#include "ast.hpp"

const std::string type_string[] = {"unit", "int", "float", "bool", "char"};

// Stores the amount of spaces to be inserted before printing the node
int spaces = 0; 

std::ostream &operator<<(std::ostream &out, const AST &t)
{
    t.printOn(out);
    return out;
}

void UnknownType::printOn(std::ostream &out) const
{
    out << "UnknownType(" << TG->stringifyType() << ")";
}
void BasicType::printOn(std::ostream &out) const
{
    out << type_string[static_cast<int>(t)];
}
void FunctionType::printOn(std::ostream &out) const
{
    out << *lhtype << "->" << *rhtype;
}
void ArrayType::printOn(std::ostream &out) const
{
    out << "TYPE_array(" << dimensions << ", " << *elem_type << ")";
}
void RefType::printOn(std::ostream &out) const
{
    out << "TYPE_ref(" << *ref_type << ")";
}
void CustomType::printOn(std::ostream &out) const
{
    out << "CustomType(" << id << ")";
}

void Constr::printOn(std::ostream &out) const
{
    out << "Constr(" << Id;

    for (Type *t : type_list)
    {
        out << ", " << *t;
    }

    out << ")";
}
void Par::printOn(std::ostream &out) const
{
    out << "Par(" << id << ", " << *T << ")";
}

void Tdef::printOn(std::ostream &out) const
{
    out << "Tdef(" << id << ", ";

    bool first = true;
    for (Constr *c : constr_list)
    {
        if (!first)
            out << ", ";
        else
            first = false;

        out << *c;
    }

    out << ")";
}
void Constant::printOn(std::ostream &out) const
{
    out << "Constant(" << id << ", " << *T << ", " << *expr << ")";
}
void Function::printOn(std::ostream &out) const
{
    out << "Function(" << id;
    for (Par *p : par_list)
    {
        out << ", " << *p;
    }
    out << ", " << *T << ", " << *expr << ")";
}
void Array::printOn(std::ostream &out) const
{
    out << "Array(" << id;
    if (!expr_list.empty())
    {
        out << "[";

        bool first = true;
        for (Expr *e : expr_list)
        {
            if (!first)
                out << ", ";
            else
                first = false;

            out << *e;
        }
    }
    out << "], " << *T << ")";
}
void Variable::printOn(std::ostream &out) const
{
    out << "Variable(" << id << ", " << *T << ", "
        << ")";
}

void Letdef::printOn(std::ostream &out) const
{
    out << "Let(";
    if (recursive)
        out << "rec ";

    bool first = true;
    for (DefStmt *d : def_list)
    {
        if (!first)
            out << "and ";
        else
            first = false;

        out << *d;
    }

    out << ")";
}
void Typedef::printOn(std::ostream &out) const
{
    out << "Type(";

    bool first = true;
    for (DefStmt *t : tdef_list)
    {
        if (!first)
            out << ", ";
        else
            first = false;

        out << *t;
    }

    out << ")";
}
void Program::printOn(std::ostream &out) const
{
    out << "Definition(";

    bool first = true;
    for (Definition *d : definition_list)
    {
        if (!first)
            out << ",";
        else
            first = false;
        out << *d;
    }

    out << ")";
}

void LetIn::printOn(std::ostream &out) const
{
    out << "LetIN(" << *letdef << ", " << *expr << ")";
}

void String_literal::printOn(std::ostream &out) const
{
    out << "String(" << originalStr << ")";
}
void Char_literal::printOn(std::ostream &out) const
{
    out << "Char(" << c_string << ")";
}
void Bool_literal::printOn(std::ostream &out) const
{
    out << "Bool(" << b << ")";
}
void Float_literal::printOn(std::ostream &out) const
{
    out << "Float(" << d << ")";
}
void Int_literal::printOn(std::ostream &out) const
{
    out << "Int(" << n << ")";
}
void Unit_literal::printOn(std::ostream &out) const
{
    out << "Unit";
}

void BinOp::printOn(std::ostream &out) const
{
    out << "BinOp(" << *lhs << ", " << op << ", " << *rhs << ")";
}
void UnOp::printOn(std::ostream &out) const
{
    out << "UnOp(" << op << ", " << *expr << ")";
}
void New::printOn(std::ostream &out) const
{
    out << "New(" << *new_type << ")";
}

void While::printOn(std::ostream &out) const
{
    out << "While(" << *cond << ", " << *body << ")";
}
void For::printOn(std::ostream &out) const
{
    out << "For(" << id << ", " << *start << ", " << step << *finish << ", " << *body << ")";
}
void If::printOn(std::ostream &out) const
{
    out << "If(" << *cond << ", " << *body;

    if (else_body)
        out << ", " << *else_body;

    out << ")";
}

void Dim::printOn(std::ostream &out) const
{
    out << "Dim(";
    if (dim)
        out << *dim << ", ";
    out << id << ")";
}

void ConstantCall::printOn(std::ostream &out) const
{
    out << "ConstantCall(" << id << ")";
}
void FunctionCall::printOn(std::ostream &out) const
{
    out << "FunctionCall(" << id;
    for (Expr *e : expr_list)
    {
        out << ", " << *e;
    }
    out << ")";
}
void ConstructorCall::printOn(std::ostream &out) const
{
    out << "ConstructorCall(" << Id;
    for (Expr *e : expr_list)
    {
        out << ", " << *e;
    }
    out << ")";
}
void ArrayAccess::printOn(std::ostream &out) const
{
    out << "ArrayAccess(" << id << "[";

    bool first = true;
    for (Expr *e : expr_list)
    {
        if (!first)
            out << ", ";
        else
            first = false;

        out << *e;
    }

    out << "])";
}

void PatternLiteral::printOn(std::ostream &out) const
{
    out << "PatternLiteral(" << *literal << ")";
}
void PatternId::printOn(std::ostream &out) const
{
    out << "PatternId(" << id << ")";
}
void PatternConstr::printOn(std::ostream &out) const
{
    out << "PatternConstr(" << Id;
    for (Pattern *p : pattern_list)
    {
        out << ", " << *p;
    }
    out << ")";
}

void Clause::printOn(std::ostream &out) const
{
    out << "Clause(" << *pattern << ", " << *expr << ")";
}
void Match::printOn(std::ostream &out) const
{
    out << "Match(" << *toMatch;
    for (Clause *c : clause_list)
    {
        out << ", " << *c;
    }
    out << ")";
}
