#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "symbol.hpp"
#include "types.hpp"

const std::string type_string[] = {"unit", "int", "float", "bool", "char"};
enum class type
{
    TYPE_unit,
    TYPE_int,
    TYPE_float,
    TYPE_bool,
    TYPE_char
};
enum class category
{
    CATEGORY_basic,
    CATEGORY_function,
    CATEGORY_array,
    CATEGORY_ref,
    CATEGORY_custom,
    CATEGORY_unknown
};

void yyerror(const char *msg);

/* TypeGraphs of basic types useful for type checking ***************/

extern TypeGraph *type_unit;
extern TypeGraph *type_int;
extern TypeGraph *type_float;
extern TypeGraph *type_bool;
extern TypeGraph *type_char;

/********************************************************************/

class AST
{
protected:
    int line_number;

public:
    AST()
    {
        line_number = yylineno;
    }
    virtual ~AST()
    {
    }
    virtual void printOn(std::ostream &out) const = 0;
    virtual void sem()
    {
    }
    virtual void printError(std::string s)
    {
        std::cout << "Error at line " << line_number << ": " << s << std::endl;
        exit(1);
    }
};

inline std::ostream &operator<<(std::ostream &out, const AST &t)
{
    t.printOn(out);
    return out;
}

/* Type classes *****************************************************/
// These classes are used when the name of a type apears in the code

class Type : public AST
{
protected:
    category c; // Shows what kind of object it is
public:
    Type(category c)
        : c(c) {}
    category get_category()
    {
        return c;
    }
    bool compare_category(category _c)
    {
        return c == _c;
    }
    virtual bool compare_basic_type(type t)
    {
        return false;
    }
    virtual TypeGraph *get_TypeGraph() = 0;
    friend bool compare_categories(Type *T1, Type *T2)
    {
        return (T1->c == T2->c);
    }
};
class UnknownType : public Type
{
protected:
    std::string temp_name;

public:
    UnknownType()
        : Type(category::CATEGORY_unknown) {}
    virtual TypeGraph *get_TypeGraph() override
    {
        return tt.lookupType(temp_name)->getTypeGraph();
    }
    virtual std::string stringify() 
    {
        return temp_name;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "UnknownType()";
    }
};
class BasicType : public Type
{
protected:
    // Shows which BasicType it is
    type t;

public:
    BasicType(type t)
        : Type(category::CATEGORY_basic), t(t) {}
    virtual bool compare_basic_type(type _t) override
    {
        return t == _t;
    }
    virtual TypeGraph *get_TypeGraph() override
    {
        return tt.lookupType(type_string[(int)t])->getTypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << type_string[static_cast<int>(t)];
    }
};
class FunctionType : public Type
{
private:
    Type *lhtype, *rhtype;

public:
    FunctionType(Type *lhtype = new UnknownType, Type *rhtype = new UnknownType)
        : Type(category::CATEGORY_function), lhtype(lhtype), rhtype(rhtype) {}
    virtual TypeGraph *get_TypeGraph() override
    {
        FunctionTypeGraph *f;
        TypeGraph *l = lhtype->get_TypeGraph();
        TypeGraph *r = rhtype->get_TypeGraph();

        if (r->isFunction())
        {
            f = dynamic_cast<FunctionTypeGraph *>(r);
        }

        else
        {
            f = new FunctionTypeGraph(r);
        }

        f->addParam(l);
        return f;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << *lhtype << "->" << *rhtype;
    }
};
class ArrayType : public Type
{
private:
    int dimensions;
    Type *elem_type;

public:
    ArrayType(int dimensions = 1, Type *elem_type = new UnknownType)
        : Type(category::CATEGORY_array), dimensions(dimensions), elem_type(elem_type) {}
    virtual TypeGraph *get_TypeGraph() override
    {
        return new ArrayTypeGraph(dimensions, elem_type->get_TypeGraph());
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "TYPE_array(" << dimensions << ", " << *elem_type << ")";
    }
};
class RefType : public Type
{
    /* NOTE: MUST NOT BE ArrayType !! */
private:
    Type *ref_type;

public:
    RefType(Type *ref_type = new BasicType(type::TYPE_int))
        : Type(category::CATEGORY_ref), ref_type(ref_type) {}
    virtual TypeGraph *get_TypeGraph() override
    {
        return new RefTypeGraph(ref_type->get_TypeGraph());
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "TYPE_ref(" << *ref_type << ")";
    }
};
class CustomType : public Type
{
private:
    std::string id;

public:
    CustomType(std::string *id)
        : Type(category::CATEGORY_custom), id(*id) {}
    virtual TypeGraph *get_TypeGraph()
    {
        return tt.lookupType(id)->getTypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "CustomType(" << id << ")";
    }
};

/* Basic Abstract Classes *******************************************/

class Expr : public AST
{
protected:
    Type *T;
    TypeGraph *TG;

    // dynamic shows whether an expression
    // was created with new (Probably bad idea)
    bool dynamic = false;

public:
    TypeGraph *get_TypeGraph()
    {
        return TG;
    }
    void type_check(TypeGraph *t, std::string msg = "Type mismatch", bool negation = false)
    {
        if ((!negation && !TG->equals(t)) || (negation && TG->equals(t)))
        {
            printError(msg);
        }
    }
    void type_check(std::vector<TypeGraph *> TypeGraph_list, std::string msg = "Type mismatch", bool negation = false)
    {
        bool flag;

        if (!negation)
        {
            flag = false;

            // TG must be in the list
            for (TypeGraph *t : TypeGraph_list)
            {
                if (TG->equals(t))
                    flag = true;
            }
        }
        else
        {
            flag = true;

            // TG must not be in the list
            for (TypeGraph *t : TypeGraph_list)
            {
                if (TG->equals(t))
                    flag = false;
            }
        }

        if (!flag)
        {
            printError(msg);
        }
    }
    friend void same_type(Expr *e1, Expr *e2, std::string msg = "Type mismatch")
    {
        e1->type_check(e2->TG, msg);
    }
};

class Identifier : public Expr
{
protected:
    std::string name;
};

/********************************************************************/

class Constr : public AST
{
private:
    std::string Id;
    std::vector<Type *> type_list;

public:
    Constr(std::string *Id, std::vector<Type *> *t)
        : Id(*Id), type_list(*t) {}
    void add_Id_to_ct(TypeEntry *te)
    {
        ConstructorEntry *c = ct.insertConstructor(Id);
        for (Type *t : type_list)
        {
            c->addType(t->get_TypeGraph());
        }

        te->addConstructor(c);
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Constr(" << Id;

        for (Type *t : type_list)
        {
            out << ", " << *t;
        }

        out << ")";
    }
};

/********************************************************************/

class Par : public AST
{
private:
    std::string id;
    Type *T;

public:
    Par(std::string *id, Type *t = new UnknownType)
        : id(*id), T(t) {}
    /*SymbolEntry* get_SymbolEntry() { return new SymbolEntry(id, T->get_TypeGraph()); }*/
    void insert_id_to_st()
    {
        st.insertBasic(id, T->get_TypeGraph());
    }
    TypeGraph *get_TypeGraph()
    {
        return T->get_TypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Par(" << id << ", " << *T << ")";
    }
};

/********************************************************************/

class DefStmt : public AST
{
protected:
    std::string id;

public:
    DefStmt(std::string id)
        : id(id) {}
    virtual void insert_id_to_st()
    {
    }
    virtual void insert_id_to_tt()
    {
    }
};
class Tdef : public DefStmt
{
private:
    std::vector<Constr *> constr_list;

public:
    Tdef(std::string *id, std::vector<Constr *> *c)
        : DefStmt(*id), constr_list(*c) {}
    virtual void insert_id_to_tt() override
    {
        // Insert custom type to type table
        tt.insertType(id);
    }
    virtual void sem() override
    {
        TypeEntry *t = tt.lookupType(id);

        for (Constr *c : constr_list)
        {
            c->add_Id_to_ct(t);
        }
    }
    virtual void printOn(std::ostream &out) const override
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
};
class Def : public DefStmt
{
protected:
    // T is the Type of the defined identifier
    Type *T;

public:
    Def(std::string id, Type *t)
        : DefStmt(id), T(t) {}
    Type *get_type()
    { /* NOTE: Returns the pointer */
        return T;
    }
};
class Constant : public Def
{
protected:
    Expr *expr;

public:
    Constant(std::string *id, Expr *e, Type *t = new UnknownType)
        : Def(*id, t), expr(e) {}
    virtual void sem() override
    {
        std::string err = "Must be of specified type " + T->get_TypeGraph()->stringifyType();
        expr->sem();
        expr->type_check(T->get_TypeGraph(), err);
    }
    virtual void insert_id_to_st() override
    {
        st.insertBasic(id, T->get_TypeGraph());
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Constant(" << id << ", " << *T << ", " << *expr << ")";
    }
};
class Function : public Constant
{
private:
    std::vector<Par *> par_list;

public:
    Function(std::string *id, std::vector<Par *> *p, Expr *e, Type *t = new UnknownType)
        : Constant(id, e, t), par_list(*p) {}
    virtual void sem() override
    {
        // New scope for the body of the function where the parameters will be inserted
        st.openScope();

        // Insert parameters to symbol table
        for (Par *p : par_list)
        {
            p->insert_id_to_st();
        }

        // Check the type of the expression (and call sem)
        std::string err = "Function body must be of specified type " + T->get_TypeGraph()->stringifyType();
        expr->sem();
        expr->type_check(T->get_TypeGraph(), err);

        // Close the scope
        st.closeScope();
    }
    virtual void insert_id_to_st() override
    {
        // Insert Function id to symbol table
        FunctionEntry *F = st.insertFunction(id, T->get_TypeGraph());

        // Add the parameters to the entry
        for (Par *p : par_list)
        {
            F->addParam(p->get_TypeGraph());
        }
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Function(" << id;
        for (Par *p : par_list)
        {
            out << ", " << *p;
        }
        out << ", " << *T << ", " << *expr << ")";
    }
};
class Mutable : public Def
{
public:
    Mutable(std::string id, Type *T)
        : Def(id, T) {}
};
class Array : public Mutable
{
private:
    std::vector<Expr *> expr_list;

public:
    Array(std::string *id, std::vector<Expr *> *e, Type *T = new UnknownType)
        : Mutable(*id, T), expr_list(*e) {}
    virtual void sem() override
    {
        // All dimension sizes are of type integer
        for (Expr *e : expr_list)
        {
            e->type_check(type_int, "Array dimension sizes must be int");
        }
    }
    int get_dimensions()
    {
        return expr_list.size();
    }
    virtual void insert_id_to_st()
    {
        int d = get_dimensions();

        RefTypeGraph *contained_type = new RefTypeGraph(T->get_TypeGraph());

        st.insertArray(id, contained_type, d);
    }
    virtual void printOn(std::ostream &out) const override
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
        out << ", " << *T << ")";
    }
};
class Variable : public Mutable
{
public:
    Variable(std::string *id, Type *T = new UnknownType)
        : Mutable(*id, T) {}
    virtual void insert_id_to_st() override
    {
        st.insertRef(id, T->get_TypeGraph(), true, false);
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Variable(" << id << ", " << *T << ", "
            << ")";
    }
};

/********************************************************************/

class Definition : public AST
{
    // virtual void append(DefStmt *d) = 0;
};
class Letdef : public Definition
{
private:
    bool recursive;
    std::vector<DefStmt *> def_list;

public:
    Letdef(std::vector<DefStmt *> *d, bool rec = false)
        : recursive(rec), def_list(*d) {}
    virtual void sem() override
    {
        // Recursive
        if (recursive)
        {
            // Insert identifiers to symbol table before all the definitions
            for (DefStmt *d : def_list)
            {
                d->insert_id_to_st();
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
                d->insert_id_to_st();
            }
        }
    }
    virtual void printOn(std::ostream &out) const override
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
};
class Typedef : public Definition
{
private:
    std::vector<DefStmt *> tdef_list;

public:
    Typedef(std::vector<DefStmt *> *t)
        : tdef_list(*t) {}
    virtual void sem() override
    {
        // Insert identifiers to type table before all the definitions
        for (DefStmt *td : tdef_list)
        {
            td->insert_id_to_tt();
        }

        // Semantically analyse definitions
        for (DefStmt *td : tdef_list)
        {
            td->sem();
        }
    }
    virtual void printOn(std::ostream &out) const override
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
};

/********************************************************************/

class Program : public AST
{
private:
    std::vector<Definition *> definition_list;

public:
    ~Program()
    {
    }
    Program()
        : definition_list() {}
    virtual void sem() override
    {
        for (Definition *d : definition_list)
        {
            d->sem();
        }
    }
    void append(Definition *d)
    {
        definition_list.push_back(d);
    }
    virtual void printOn(std::ostream &out) const override
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
};

/* Expressions ******************************************************/
// class Expr: public AST (used to be here)
// class Identifier: public Expr (used to be here)

class LetIn : public Expr
{
private:
    Definition *letdef;
    Expr *expr;

public:
    LetIn(Definition *letdef, Expr *expr)
        : letdef(letdef), expr(expr) {}
    virtual void sem() override
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
    virtual void printOn(std::ostream &out) const override
    {
        out << "LetIN(" << *letdef << ", " << *expr << ")";
    }
};

class Id_upper : public Identifier
{
public:
    Id_upper(std::string *s)
    {
        name = *s;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Id(" << name << ")";
    }
};
class Id_lower : public Identifier
{
public:
    Id_lower(std::string *s)
    {
        name = *s;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "id(" << name << ")";
    }
};

class Literal : public Expr
{
};
class String_literal : public Literal
{
private:
    std::string s;

public:
    String_literal(std::string *s)
        : s(*s) {}
    virtual void sem() override
    {
        TG = new ArrayTypeGraph(1, type_char);
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "String(" << s << ")";
    }
};
class Char_literal : public Literal
{
private:
    std::string c_string;
    char c;

public:
    Char_literal(std::string *c_string)
        : c_string(*c_string)
    {
        c = getChar(*c_string);
    }
    // Recognise character from string
    char getChar(std::string c)
    {
        char ans = 0;

        // Normal character
        if (c[1] != '\\')
        {
            ans = c[1];
        }

        // '\xnn'
        else if (c[2] == 'x')
        {
            const char hex[2] = {c[3], c[4]};
            ans = strtol(hex, nullptr, 16);
        }

        // Escape secuence
        else
        {
            switch (c[2])
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
    virtual void sem() override
    {
        TG = type_char;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Char(" << c_string << ")";
    }
};
class Bool_literal : public Literal
{
private:
    bool b;

public:
    Bool_literal(bool b)
        : b(b) {}
    virtual void sem() override
    {
        TG = type_bool;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Bool(" << b << ")";
    }
};
class Float_literal : public Literal
{
private:
    double d;

public:
    Float_literal(double d)
        : d(d) {}
    virtual void sem() override
    {
        TG = type_float;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Float(" << d << ")";
    }
};
class Int_literal : public Literal
{
private:
    int n;

public:
    Int_literal(int n)
        : n(n) {}
    int get_int()
    {
        return n;
    }
    virtual void sem() override
    {
        TG = type_int;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Int(" << n << ")";
    }
};
class Unit_literal : public Literal
{
public:
    Unit_literal() {}
    virtual void sem() override
    {
        TG = type_unit;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Unit";
    }
};

class BinOp : public Expr
{
private:
    Expr *lhs, *rhs;
    int op;

public:
    BinOp(Expr *e1, int op, Expr *e2)
        : lhs(e1), rhs(e2), op(op) {}
    virtual void sem() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "BinOp(" << *lhs << ", " << op << ", " << *rhs << ")";
    }
};
class UnOp : public Expr
{
private:
    Expr *expr;
    int op;

public:
    UnOp(int op, Expr *e)
        : expr(e), op(op) {}
    virtual void sem() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "UnOp(" << op << ", " << *expr << ")";
    }
};
class New : public Expr
{
    Type *new_type;

public:
    New(Type *t) : new_type(t) {}
    virtual void sem() override
    {
        TypeGraph *t = new_type->get_TypeGraph();

        // Array type not allowed
        if (t->isArray())
        {
            printError("Array type cannot be allocated with new");
        }

        TG = new RefTypeGraph(t, true, true);
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "New(" << *T << ")";
    }
};

class While : public Expr
{
private:
    Expr *cond, *body;

public:
    While(Expr *e1, Expr *e2)
        : cond(e1), body(e2) {}
    virtual void sem() override
    {
        // Typecheck
        cond->type_check(type_bool, "While condition must be bool");
        body->type_check(type_unit, "While body must be unit");

        TG = type_unit;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "While(" << *cond << ", " << *body << ")";
    }
};
class For : public Expr
{
private:
    std::string id;
    std::string step;
    Expr *start, *finish, *body;

public:
    For(std::string *id, Expr *e1, std::string s, Expr *e2, Expr *e3)
        : id(*id), step(s), start(e1), finish(e2), body(e3) {}
    virtual void sem() override
    {
        // Create new scope for counter and add it
        st.openScope();
        st.insertBasic(id, type_int);

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
    virtual void printOn(std::ostream &out) const override
    {
        out << "For(" << id << ", " << *start << ", " << step << *finish << ", " << *body << ")";
    }
};
class If : public Expr
{
private:
    Expr *cond, *body, *else_body;

public:
    If(Expr *e1, Expr *e2, Expr *e3 = nullptr) : cond(e1), body(e2), else_body(e3) {}
    virtual void sem() override
    {
        cond->sem();
        cond->type_check(type_bool, "Condition of if must be bool");

        // If there is no else just semantically analyse body
        if (else_body == nullptr)
        {
            body->sem();
        }

        // If there is else then check if the types match
        else
        {
            body->sem();
            else_body->sem();
            same_type(body, else_body, "Return value of if and else must be same type");
        }

        // The type of the body is the type of the If
        TG = body->get_TypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "If(" << *cond << ", " << *body;

        if (else_body)
            out << ", " << *else_body;

        out << ")";
    }
};

class Dim : public Expr
{
private:
    Int_literal *dim;
    std::string id;

public:
    Dim(std::string *id, Int_literal *dim = new Int_literal(1))
        : dim(dim), id(*id) {}
    virtual void sem() override
    {
        // Lookup the array
        ArrayEntry *arr = st.lookupArray(id);

        // Get the number of the dimension
        int i = dim->get_int();

        // Check if i is withing the correct bounds
        if (i < 1 && i > arr->getTypeGraph()->getDimensions())
        {
            printError("Index out of bounds");
        }

        TG = type_int;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Dim(";
        if (dim)
            out << *dim << ", ";
        out << id << ")";
    }
};

class ConstantCall : public Expr
{
protected:
    std::string id;

public:
    ConstantCall(std::string *id)
        : id(*id) {}
    virtual void sem() override
    {
        SymbolEntry *s = st.lookup(id);
        TG = s->getTypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "ConstantCall(" << id << ")";
    }
};
class FunctionCall : public ConstantCall
{
private:
    std::vector<Expr *> expr_list;

public:
    FunctionCall(std::string *id, std::vector<Expr *> *expr_list)
        : ConstantCall(id), expr_list(*expr_list) {}
    virtual void sem() override
    {
        FunctionEntry *f = st.lookupFunction(id);
        TypeGraph *t = f->getTypeGraph();

        int count = t->getParamCount();
        if (count != (int)expr_list.size())
        {
            printError("Partial function call not allowed");
        }

        std::string err = "Type mismatch on parameter No. ";
        TypeGraph *correct_t;
        for (int i = 0; i < count; i++)
        {
            correct_t = t->getParamType(i);

            expr_list[i]->sem();
            expr_list[i]->type_check(correct_t, err + std::to_string(i));
        }

        TG = t->getResultType();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "FunctionCall(" << id;
        for (Expr *e : expr_list)
        {
            out << ", " << *e;
        }
        out << ")";
    }
};
class ConstructorCall : public Expr
{
private:
    std::string Id;
    std::vector<Expr *> expr_list;

public:
    ConstructorCall(std::string *Id, std::vector<Expr *> *expr_list = new std::vector<Expr *>())
        : Id(*Id), expr_list(*expr_list) {}
    virtual void sem()
    {
        ConstructorEntry *c = ct.lookupConstructor(Id);
        TypeGraph *t = c->getTypeGraph();

        int count = t->getFieldCount();
        if (count != (int)expr_list.size())
        {
            printError("Partial constructor call not allowed");
        }

        std::string err = "Type mismatch on field No. ";
        TypeGraph *correct_t;
        for (int i = 0; i < count; i++)
        {
            correct_t = t->getFieldType(i);

            expr_list[i]->sem();
            expr_list[i]->type_check(correct_t, err + std::to_string(i));
        }

        TG = c->getTypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "ConstructorCall(" << Id;
        for (Expr *e : expr_list)
        {
            out << ", " << *e;
        }
        out << ")";
    }
};
class ArrayAccess : public Expr
{
private:
    std::string id;
    std::vector<Expr *> expr_list;

public:
    ArrayAccess(std::string *id, std::vector<Expr *> *expr_list)
        : id(*id), expr_list(*expr_list) {}
    virtual void sem() override
    {
        ArrayEntry *a = st.lookupArray(id);
        TypeGraph *t = a->getTypeGraph();

        int count = t->getDimensions();
        if (count != (int)expr_list.size())
        {
            printError("Partial array call not allowed");
        }

        TypeGraph *temp_e;
        for (int i = 0; i < count; i++)
        {
            expr_list[i]->sem();
            expr_list[i]->type_check(type_int, "Array indices can only be int");
        }

        TG = t->getContainedType();
    }
    virtual void printOn(std::ostream &out) const override
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
};

/* Match ************************************************************/

class Pattern : public AST
{
public:
    // Checks whether the pattern is valid for TypeGraph *t
    virtual void checkPatternTypeGraph(TypeGraph *t)
    {
    }
};
class PatternLiteral : public Pattern
{
protected:
    Literal *literal;

public:
    PatternLiteral(Literal *l)
        : literal(l) {}
    virtual void checkPatternTypeGraph(TypeGraph *t) override
    {
        if (!t->equals(literal->get_TypeGraph()))
        {
            printError("Literal is not a valid pattern for given type");
        }
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternLiteral(" << *literal << ")";
    }
};
class PatternId : public Pattern
{
protected:
    std::string id;

public:
    PatternId(std::string *id)
        : id(*id) {}
    virtual void checkPatternTypeGraph(TypeGraph *t) override
    {
        // Insert a new symbol with name id and type the same as that of e
        st.insertBasic(id, t);
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternId(" << id << ")";
    }
};
class PatternConstr : public Pattern
{
protected:
    std::string Id;
    std::vector<Pattern *> pattern_list;

public:
    PatternConstr(std::string *Id, std::vector<Pattern *> *p_list = new std::vector<Pattern *>())
        : Id(*Id), pattern_list(*p_list) {}
    virtual void checkPatternTypeGraph(TypeGraph *t) override
    {
        ConstructorEntry *c = ct.lookupConstructor(Id);
        TypeGraph *c_TypeGraph = c->getTypeGraph();

        int count = c_TypeGraph->getFieldCount();
        if (count != (int)pattern_list.size())
        {
            printError("Partial constructor pattern not allowed");
        }

        TypeGraph *correct_t;
        for (int i = 0; i < count; i++)
        {
            correct_t = c_TypeGraph->getFieldType(i);

            pattern_list[i]->checkPatternTypeGraph(correct_t);
        }
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternConstr(" << Id;
        for (Pattern *p : pattern_list)
        {
            out << ", " << *p;
        }
        out << ")";
    }
};

class Clause : public AST
{
private:
    Pattern *pattern;
    Expr *expr;

    // Will be filled by the Match's sem
    TypeGraph *correctPatternTypeGraph = nullptr;

public:
    Clause(Pattern *p, Expr *e)
        : pattern(p), expr(e) {}
    virtual void sem() override
    {
        // Open new scope just in case of PatternId
        st.openScope();

        // Check whether the pattern is valid for given expression
        if (correctPatternTypeGraph == nullptr)
        {
            printError("Don't know the expected type of e");
        }
        else
        {
            pattern->checkPatternTypeGraph(correctPatternTypeGraph);
        }

        // Semantically analyse expression
        expr->sem();

        // Close the scope
        st.closeScope();
    }
    void set_correctPatternTypeGraph(TypeGraph *t)
    {
        correctPatternTypeGraph = t;
    }
    TypeGraph *get_exprTypeGraph()
    {
        return expr->get_TypeGraph();
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Clause(" << *pattern << ", " << *expr << ")";
    }
};
class Match : public Expr
{
private:
    Expr *toMatch;
    std::vector<Clause *> clause_list;

public:
    Match(Expr *e, std::vector<Clause *> *c)
        : toMatch(e), clause_list(*c) {}
    virtual void sem()
    {
        // Semantically analyse expression
        toMatch->sem();

        // Get the TypeGraph t of e, must be the same as patterns
        TypeGraph *t = toMatch->get_TypeGraph();

        // Will be used during the loop to check all possible results
        TypeGraph *temp;

        // On the first loop temp will be assigned a value
        bool first = true;

        // Semantically analyse every clause
        for (Clause *c : clause_list)
        {
            c->set_correctPatternTypeGraph(t);
            c->sem();

            if (first)
            {
                temp = c->get_exprTypeGraph();
                first = false;
                continue;
            }
            else if (temp->equals(c->get_exprTypeGraph()))
            {
                temp = c->get_exprTypeGraph();
            }
            else
            {
                printError("Results of match have different types");
            }
        }

        // Reached the end so all the results have the same type
        TG = temp;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "Match(" << *toMatch;
        for (Clause *c : clause_list)
        {
            out << ", " << *c;
        }
        out << ")";
    }
};
