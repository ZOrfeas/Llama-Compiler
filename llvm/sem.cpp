#include "ast.hpp"

TypeGraph *Expr::get_TypeGraph()
{
    return TG;
}
void Expr::type_check(TypeGraph *t, std::string msg = "Type mismatch")
{
    checkTypeGraphs(TG, t, msg + ", " + TG->stringifyTypeClean() + " given.");
}
void Expr::checkIntCharFloat(std::string msg = "Must be int, char or float")
{
    if (!TG->isUnknown())
    {
        if (!TG->equals(type_int) && !TG->equals(type_char) && !TG->equals(type_float))
        {
            printError(msg);
        }
    }
    else
    {
        TG->setIntCharFloat();
    }
}
void same_type(Expr *e1, Expr *e2, std::string msg = "Type mismatch")
{
    e1->type_check(e2->TG, msg);
}

void Constr::add_Id_to_ct(TypeEntry *te)
{
    ConstructorEntry *c = insertConstructorToConstructorTable(Id);
    for (Type *t : type_list)
    {
        c->addType(t->get_TypeGraph());
    }
    te->addConstructor(c);
}

void Par::insert_id_to_st()
{
    insertBasicToSymbolTable(id, T->get_TypeGraph());

    addToIdList(id);
}
TypeGraph *Par::get_TypeGraph()
{
    return T->get_TypeGraph();
}
std::string Par::getId() 
{ 
    return id; 
}

