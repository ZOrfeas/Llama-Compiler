#include "ast.hpp"
#include "parser.hpp"

/*********************************/
/**          AST                 */
/*********************************/

void AST::sem()
{

}
void AST::checkTypeGraphs(TypeGraph *t1, TypeGraph *t2, std::string msg)
{
    // if (!t1->isUnknown() && !t2->isUnknown())
    // {
    //     if (!t1->equals(t2))
    //     {
    //         printError(msg);
    //     }
    // }
    // else
    // {
    inf.addConstraint(t1, t2, line_number, msg);
    // }
}
void AST::printError(std::string msg)
{
    std::string intro = "Error at line " + std::to_string(line_number) + ": ";
    printColorString(intro, intro.size(), 1, 31);
    std::cout << msg << std::endl;
    exit(1);
}
void AST::insertToTable()
{
}
void AST::insertBasicToSymbolTable(std::string id, TypeGraph *t)
{
    st.insertBasic(id, t);
}
void AST::insertRefToSymbolTable(std::string id, TypeGraph *t)
{
    st.insertRef(id, t);
}
void AST::insertArrayToSymbolTable(std::string id, TypeGraph *contained_type, int d)
{
    st.insertArray(id, contained_type, d);
}
FunctionEntry *AST::insertFunctionToSymbolTable(std::string id, TypeGraph *t)
{
    return st.insertFunction(id, t);
}
void AST::insertTypeToTypeTable(std::string id)
{
    if (!tt.insertType(id))
    {
        printError("Type " + id + " has already been defined");
    }
}
ConstructorEntry *AST::insertConstructorToConstructorTable(std::string Id)
{
    ConstructorEntry *c = ct.insertConstructor(Id);
    if (!c)
    {
        std::string type = lookupConstructorFromContstructorTable(Id)->getTypeGraph()->getCustomType()->stringifyTypeClean();
        printError("Constructor " + Id + " already belongs to type " + type);
    }

    return c;
}
SymbolEntry *AST::lookupBasicFromSymbolTable(std::string id)
{
    SymbolEntry *s = st.lookup(id, false);
    if (!s)
    {
        printError("Identifier " + id + " not found");
    }

    return s;
}
ArrayEntry *AST::lookupArrayFromSymbolTable(std::string id)
{
    ArrayEntry *s = st.lookupArray(id, false);
    if (!s)
    {
        printError("Array identifier " + id + " not found");
    }

    return s;
}
TypeEntry *AST::lookupTypeFromTypeTable(std::string id)
{
    TypeEntry *t = tt.lookupType(id, false);
    if (!t)
    {
        printError("Type " + id + " not found");
    }

    return t;
}
ConstructorEntry *AST::lookupConstructorFromContstructorTable(std::string Id)
{
    ConstructorEntry *c = ct.lookupConstructor(Id, false);
    if (!c)
    {
        printError("Constructor " + Id + " not found");
    }

    return c;
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

/*********************************/
/**          Type                */
/*********************************/

category Type::get_category()
{
    return c;
}
bool Type::compare_category(category _c)
{
    return c == _c;
}
bool Type::compare_basic_type(type t)
{
    return false;
}
bool compare_categories(Type *T1, Type *T2)
{
    return (T1->c == T2->c);
}
bool BasicType::compare_basic_type(type _t)
{
    return t == _t;
}

TypeGraph *BasicType::get_TypeGraph()
{
    if (!TG)
        TG = lookupTypeFromTypeTable(type_string[(int)t])->getTypeGraph();

    return TG;
}
TypeGraph *UnknownType::get_TypeGraph()
{
    return TG;
}
TypeGraph *FunctionType::get_TypeGraph()
{
    if (!TG)
    {
        TypeGraph *l = lhtype->get_TypeGraph();
        TypeGraph *r = rhtype->get_TypeGraph();

        // No type inference needed here because FunctionType is called only if the type is given
        if (r->isFunction())
        {
            TG = r;
        }
        else
        {
            TG = new FunctionTypeGraph(r);
        }

        // Operator -> is right associative
        // so the parameters will be added
        // from last to first
        TG->addParam(l, false);
    }

    return TG;
}
TypeGraph *ArrayType::get_TypeGraph()
{
    if (!TG)
        TG = new ArrayTypeGraph(dimensions, new RefTypeGraph(elem_type->get_TypeGraph()));
    return TG;
}
TypeGraph *RefType::get_TypeGraph()
{
    if (!TG)
        TG = new RefTypeGraph(ref_type->get_TypeGraph());

    return TG;
}
TypeGraph *CustomType::get_TypeGraph()
{
    if (!TG)
        TG = lookupTypeFromTypeTable(id)->getTypeGraph();

    return TG;
}

/*********************************/
/**          Definitions         */
/*********************************/

void Constr::add_Id_to_ct(TypeEntry *te)
{
    ConstructorEntry *c = insertConstructorToConstructorTable(Id);
    for (Type *t : type_list)
    {
        c->addType(t->get_TypeGraph());
    }
    te->addConstructor(c);
}

TypeGraph *Par::get_TypeGraph()
{
    return T->get_TypeGraph();
}
std::string Par::getId()
{
    return id;
}

bool DefStmt::isFunctionDefinition() const
{
    return false;
}
bool Function::isFunctionDefinition() const
{
    return true;
}

void DefStmt::insertToTable()
{
}
void Tdef::insertToTable()
{
    // Insert custom type to type table
    insertTypeToTypeTable(id);
}
void Constant::insertToTable()
{
    insertBasicToSymbolTable(id, T->get_TypeGraph());

    addToIdList(id);
}
void Function::insertToTable()
{
    // Insert Function id to symbol table
    FunctionEntry *F = insertFunctionToSymbolTable(id, T->get_TypeGraph());

    // Add the parameters to the entry
    for (Par *p : par_list)
    {
        F->addParam(p->get_TypeGraph());
    }
    TG = F->getTypeGraph();
    addToIdList(id);
}
void Par::insertToTable()
{
    insertBasicToSymbolTable(id, T->get_TypeGraph());

    addToIdList(id);
}
void Array::insertToTable()
{
    int d = get_dimensions();
    TypeGraph *t = T->get_TypeGraph();
    RefTypeGraph *contained_type = new RefTypeGraph(t);

    if (!t->isUnknown())
    {
        if (t->isArray())
        {
            printError("Array cannot contain arrays");
        }
        insertArrayToSymbolTable(id, contained_type, d);
    }
    else
    {
        TypeGraph *unknown_contained_type = new UnknownTypeGraph(false, true, false);
        insertArrayToSymbolTable(id, unknown_contained_type, d);
        inf.addConstraint(unknown_contained_type, contained_type, line_number);
    }

    addToIdList(id);
}
void Variable::insertToTable()
{
    TypeGraph *t = T->get_TypeGraph();
    TypeGraph *ref_type = new RefTypeGraph(t);

    if (!t->isUnknown())
    {
        insertRefToSymbolTable(id, t);
    }
    else
    {
        TypeGraph *unknown_ref_type = new UnknownTypeGraph(false, true, false);
        insertBasicToSymbolTable(id, unknown_ref_type);
        inf.addConstraint(unknown_ref_type, ref_type, line_number);
    }

    addToIdList(id);
}

Type *Def::get_type()
{ /* NOTE: Returns the pointer */
    return T;
}
int Array::get_dimensions()
{
    return expr_list.size();
}

void Tdef::sem()
{
    TypeEntry *t = lookupTypeFromTypeTable(id);
    for (Constr *c : constr_list)
    {
        c->add_Id_to_ct(t);
    }
}
void Constant::sem()
{
    std::string err = "Must be of specified type " + T->get_TypeGraph()->stringifyType();
    expr->sem();
    expr->type_check(T->get_TypeGraph(), err);
}
void Function::sem()
{
    // New scope for the body of the function where the parameters will be inserted
    st.openScope();

    // Insert parameters to symbol table
    for (Par *p : par_list)
    {
        p->insertToTable();
    }

    // Check the type of the expression (and call sem)
    std::string err = "Function body must be of specified type " + T->get_TypeGraph()->stringifyType();
    expr->sem();
    expr->type_check(T->get_TypeGraph(), err);

    // Close the scope
    st.closeScope();
}
void Array::sem()
{
    // All dimension sizes are of type integer
    for (Expr *e : expr_list)
    {
        e->sem();
        e->type_check(type_int, "Array dimension sizes must be int");
    }
}

void Letdef::sem()
{
    // Recursive
    if (recursive)
    {
        // Insert function names to symbol table before all the definitions
        for (DefStmt *d : def_list)
        {
            if (!d->isFunctionDefinition())
            {
                printError("Only function definitions can be recursive");
            }
            d->insertToTable();
        }

        // Semantically analyse definitions
        for (DefStmt *d : def_list)
        {
            d->sem();
        }
    }

    // Not recursive
    else
    {
        // Semantically analyse definitions
        for (DefStmt *d : def_list)
        {
            d->sem();
        }

        // Insert identifiers to symbol table after all the definitions
        for (DefStmt *d : def_list)
        {
            d->insertToTable();
        }
    }
}
void Typedef::sem()
{
    // Insert identifiers to type table before all the definitions
    for (DefStmt *td : tdef_list)
    {
        td->insertToTable();
    }

    // Semantically analyse definitions
    for (DefStmt *td : tdef_list)
    {
        td->sem();
    }
}
void Program::sem()
{
    for (Definition *d : definition_list)
    {
        d->sem();
    }
}
void LetIn::sem()
{
    // Create new scope
    st.openScope();

    // Semantically analyse letdef
    letdef->sem();

    // Semantically analyse expression
    expr->sem();

    // Close scope defined by expression
    st.closeScope();

    // The type of the LetIn is the same as that of the expression
    TG = expr->get_TypeGraph();
}


/*********************************/
/**          Expressions         */
/*********************************/

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
static char getChar(std::string c)
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
std::string String_literal::escapeChars(std::string rawStr)
{
    std::string escapedString;
    for (unsigned int i = 0; i < rawStr.size(); i++)
    {
        if (rawStr[i] != '\\')
        {
            escapedString.push_back(rawStr[i]);
        }
        else
        {
            std::string escapeSeq;
            if (rawStr[i + 1] != 'x')
            {
                escapeSeq = rawStr.substr(i, 2);
                i++;
            }
            else
            {
                escapeSeq = rawStr.substr(i, 4);
                i += 3;
            }
            escapedString.push_back(getChar(escapeSeq));
        }
    }
    return escapedString;
}
int Int_literal::get_int()
{
    return n;
}

void String_literal::sem()
{
    TG = new ArrayTypeGraph(1, new RefTypeGraph(type_char));
}
void Char_literal::sem()
{
    TG = type_char;
}
void Bool_literal::sem()
{
    TG = type_bool;
}
void Float_literal::sem()
{
    TG = type_float;
}
void Int_literal::sem()
{
    TG = type_int;
}
void Unit_literal::sem()
{
    TG = type_unit;
}

void UnOp::sem()
{
    expr->sem();
    TypeGraph *t_expr = expr->get_TypeGraph();

    switch (op)
    {
    case '+':
    case '-':
    {
        expr->type_check(type_int, "Only int allowed");

        TG = type_int;
        break;
    }
    case T_minusdot:
    case T_plusdot:
    {
        expr->type_check(type_float, "Only float allowed");

        TG = type_float;
        break;
    }
    case T_not:
    {
        expr->type_check(type_bool, "Only bool allowed");

        TG = type_bool;
        break;
    }
    case '!':
    {
        // Add constraint with ref of unknown type
        // to ensure that expr is in fact a ref
        TypeGraph *unknown = new UnknownTypeGraph(false, true, false);
        TypeGraph *ref_t = new RefTypeGraph(unknown);
        inf.addConstraint(t_expr, ref_t, line_number,
                          std::string("Expected ref, got ") + inf.deepSubstitute(t_expr)->stringifyTypeClean());

        TG = ref_t->getContainedType();
        break;
    }
    case T_delete:
    {
        // Adds constraint with ref of unknown type
        // to ensure that expr is in fact a ref
        TypeGraph *unknown_t = new UnknownTypeGraph(false, true, false);
        TypeGraph *ref_t = new RefTypeGraph(unknown_t);
        inf.addConstraint(t_expr, ref_t, line_number,
                          std::string("Expected ref, got ") + inf.deepSubstitute(t_expr)->stringifyTypeClean());

        TG = type_unit;
        break;
    }
    default:
        break;
    }
}
void BinOp::sem()
{
    lhs->sem();
    rhs->sem();

    TypeGraph *t_lhs = lhs->get_TypeGraph();
    TypeGraph *t_rhs = rhs->get_TypeGraph();

    switch (op)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case T_mod:
    {
        lhs->type_check(type_int, "Only int allowed");
        rhs->type_check(type_int, "Only int allowed");

        TG = type_int;
        break;
    }
    case T_plusdot:
    case T_minusdot:
    case T_stardot:
    case T_slashdot:
    case T_dblstar:
    {
        lhs->type_check(type_float, "Only float allowed");
        rhs->type_check(type_float, "Only float allowed");

        TG = type_float;
        break;
    }
    case T_dblbar:
    case T_dblampersand:
    {
        lhs->type_check(type_bool, "Only bool allowed");
        rhs->type_check(type_bool, "Only bool allowed");

        TG = type_bool;
        break;
    }
    case '=':
    case T_lessgreater:
    case T_dbleq:
    case T_exclameq:
    {
        // Check that they are not arrays or functions
        if (t_lhs->isArray() || t_lhs->isFunction() ||
            t_rhs->isArray() || t_rhs->isFunction())
        {
            printError("Array and Function not allowed");
        }

        // Check that they are of the same type
        same_type(lhs, rhs);

        // The result is bool
        TG = type_bool;
        break;
    }
    case '<':
    case '>':
    case T_leq:
    case T_geq:
    {
        // lhs and rhs can be one of int, char, float
        lhs->checkIntCharFloat();
        rhs->checkIntCharFloat();

        // Check that they are of the same type
        same_type(lhs, rhs);

        // Get the correct type for the result
        TG = type_bool;
        break;
    }
    case T_coloneq:
    {
        // The lhs must be a ref of the same type as the rhs
        RefTypeGraph *correct_lhs = new RefTypeGraph(t_rhs);
        lhs->type_check(correct_lhs, "Must be a ref of " + t_rhs->stringifyType());

        // Cleanup NOTE: If the new TypeGraph is
        // used for inference it should not be deleted
        // delete correct_lhs;

        // The result is unit
        TG = type_unit;
        break;
    }
    case ';':
    {
        TG = t_rhs;
    }
    default:
        break;
    }
}
void New::sem()
{
    TypeGraph *t = new_type->get_TypeGraph();

    // Array type not allowed
    if (t->isArray())
    {
        printError("Array type cannot be allocated with new");
    }

    TG = new RefTypeGraph(t);
}

void While::sem()
{
    cond->sem();
    body->sem();
    // Typecheck
    cond->type_check(type_bool, "While condition must be bool");
    body->type_check(type_unit, "While body must be unit");

    TG = type_unit;
}
void For::sem()
{
    // Create new scope for counter and add it
    st.openScope();
    insertBasicToSymbolTable(id, type_int);
    addToIdList(id);

    // Typecheck start, finish, body
    start->sem();
    finish->sem();
    body->sem();

    start->type_check(type_int, "Start value of iterator must be int");
    finish->type_check(type_int, "Finish value of iterator must be int");
    body->type_check(type_unit, "For body must be unit");

    // Close the scope
    st.closeScope();

    TG = type_unit;
}
void If::sem()
{
    cond->sem();
    cond->type_check(type_bool, "Condition of if must be bool");

    // If there is no else just semantically analyse body (must be unit)
    if (else_body == nullptr)
    {
        body->sem();
        body->type_check(type_unit, "Return type of if must be unit since there is no else");
    }

    // If there is else then check if the types match
    else
    {
        body->sem();
        else_body->sem();
        same_type(body, else_body, "Return value of if and else must be same type");
    }

    // The type of the body is always the type of the If
    TG = body->get_TypeGraph();
}

void Dim::sem()
{
    // Lookup the array
    SymbolEntry *arr = lookupBasicFromSymbolTable(id);

    // Get the number of the dimension
    int i = dim->get_int();

    // Check if i is withing the correct bounds
    if (i < 1)
    {
        printError("Index out of bounds");
    }

    ArrayTypeGraph *constraintArray =
        new ArrayTypeGraph(-1, new UnknownTypeGraph(), i);
    inf.addConstraint(arr->getTypeGraph(), constraintArray, line_number,
                      std::string("Needs array of at least ") + std::to_string(i) + " dimensions");

    TG = type_int;
}

void ConstantCall::sem()
{
    SymbolEntry *s = lookupBasicFromSymbolTable(id);
    TG = s->getTypeGraph();
}
void FunctionCall::sem()
{
    TypeGraph *definitionTypeGraph = lookupBasicFromSymbolTable(id)->getTypeGraph();
    int count;
    if (definitionTypeGraph->isUnknown())
    {
        // Create the correct FunctionTypeGraph as given by the call
        count = (int)expr_list.size();
        UnknownTypeGraph *resultTypeGraph = new UnknownTypeGraph(true, false, false);
        FunctionTypeGraph *callTypeGraph = new FunctionTypeGraph(resultTypeGraph);
        TypeGraph *argTypeGraph;
        for (int i = 0; i < count; i++)
        {
            expr_list[i]->sem();
            argTypeGraph = expr_list[i]->get_TypeGraph();
            callTypeGraph->addParam(argTypeGraph, true);
        }

        inf.addConstraint(definitionTypeGraph, callTypeGraph, line_number);

        TG = resultTypeGraph;
    }
    else if (definitionTypeGraph->isFunction())
    {
        // Check whether the call matches the definitions
        count = definitionTypeGraph->getParamCount();
        if (count != (int)expr_list.size())
        {
            printError("Partial function call not allowed");
        }

        std::string err = "Type mismatch on parameter No. ";
        TypeGraph *correct_t;
        for (int i = 0; i < count; i++)
        {
            correct_t = definitionTypeGraph->getParamType(i);

            expr_list[i]->sem();
            expr_list[i]->type_check(correct_t, err + std::to_string(i + 1) + ", " + correct_t->stringifyTypeClean() + " expected");
        }

        TG = definitionTypeGraph->getResultType();
    }
    else
    {
        printError(id + " already declared as non-function");
    }
}
void ConstructorCall::sem()
{
    ConstructorEntry *c = lookupConstructorFromContstructorTable(Id);
    constructorTypeGraph = dynamic_cast<ConstructorTypeGraph *>(c->getTypeGraph());

    int count = constructorTypeGraph->getFieldCount();
    if (count != (int)expr_list.size())
    {
        printError("Partial constructor call not allowed");
    }

    std::string err = "Type mismatch on field No. ";
    TypeGraph *correct_t;
    for (int i = 0; i < count; i++)
    {
        correct_t = constructorTypeGraph->getFieldType(i);

        expr_list[i]->sem();
        expr_list[i]->type_check(correct_t, err + std::to_string(i + 1));
    }

    TG = c->getTypeGraph();
}
void ArrayAccess::sem()
{
    int args_n = (int)expr_list.size();
    SymbolEntry *a = lookupBasicFromSymbolTable(id);
    TypeGraph *t = a->getTypeGraph();

    // If it is known check that the dimensions are correct
    // and the indices provided are integers
    if (!t->isUnknown())
    {
        if (!t->isArray())
        {
            printError("Array access attempted on " + t->stringifyTypeClean());
        }
        int count = t->getDimensions();
        if (count != args_n)
        {
            printError("Partial array call not allowed");
        }

        for (int i = 0; i < count; i++)
        {
            expr_list[i]->sem();
            expr_list[i]->type_check(type_int, "Array indices can only be int");
        }

        TG = t->getContainedType();
    }

    // If it is unknown then create the array typegraph
    // with given amount of dimensions as a constraint
    // and check that all given indices are integers
    else
    {
        TypeGraph *elemTypeGraph = new RefTypeGraph(new UnknownTypeGraph(false, true, false));
        ArrayTypeGraph *correct_array = new ArrayTypeGraph(args_n, elemTypeGraph);
        inf.addConstraint(t, correct_array, line_number);

        for (Expr *e : expr_list)
        {
            e->sem();
            e->type_check(type_int, "Array indices can only be int");
        }

        TG = elemTypeGraph;
    }
}

void Pattern::checkPatternTypeGraph(TypeGraph *t)
{
}
void PatternLiteral::checkPatternTypeGraph(TypeGraph *t)
{
    literal->sem();
    checkTypeGraphs(t, literal->get_TypeGraph(), "Literal is not a valid pattern for given type");
}
void PatternId::checkPatternTypeGraph(TypeGraph *t)
{
    // Insert a new symbol with name id and type the same as that of e
    insertBasicToSymbolTable(id, t);

    addToIdList(id);
}
void PatternConstr::checkPatternTypeGraph(TypeGraph *t)
{
    ConstructorEntry *c = lookupConstructorFromContstructorTable(Id);
    constrTypeGraph = dynamic_cast<ConstructorTypeGraph *>(c->getTypeGraph());

    // Check that toMatch is of the same type as constructor or force it to be
    checkTypeGraphs(t, constrTypeGraph, "Constructor is not of the same type as the expression to match");

    int count = constrTypeGraph->getFieldCount();
    if (count != (int)pattern_list.size())
    {
        printError("Partial constructor pattern not allowed");
    }

    TypeGraph *correct_t;
    for (int i = 0; i < count; i++)
    {
        correct_t = constrTypeGraph->getFieldType(i);

        pattern_list[i]->checkPatternTypeGraph(correct_t);
    }
}

void Match::sem()
{
    // Semantically analyse expression
    toMatch->sem();

    // Get the TypeGraph t of e, must be the same as patterns
    TypeGraph *t = toMatch->get_TypeGraph();

    // Will be used during the loop to check all possible results
    TypeGraph *prev, *curr;

    // On the first loop temp will be assigned a value
    bool first = true;

    // Semantically analyse every clause
    for (Clause *c : clause_list)
    {
        c->set_correctPatternTypeGraph(t);
        c->sem();

        if (first)
        {
            prev = c->get_exprTypeGraph();
            first = false;
            continue;
        }
        else
        {
            curr = c->get_exprTypeGraph();

            // Check that they are of the same type or force them to be
            checkTypeGraphs(prev, curr, "Results of match have different types");

            // Move prev
            prev = curr;
        }
    }

    // Reached the end so all the results have the same type
    // or the constraints will force them to be
    TG = prev;
}
void Clause::sem()
{
    // Open new scope just in case of PatternId
    st.openScope();

    // Check whether the pattern is valid for given expression
    if (correctPatternTypeGraph == nullptr)
    {
        printError("Don't know the expected type of e");
    }

    // Check pattern
    pattern->checkPatternTypeGraph(correctPatternTypeGraph);

    // Semantically analyse expression
    expr->sem();

    // Close the scope
    st.closeScope();
}
void Clause::set_correctPatternTypeGraph(TypeGraph *t)
{
    correctPatternTypeGraph = t;
}
TypeGraph *Clause::get_exprTypeGraph()
{
    return expr->get_TypeGraph();
}
