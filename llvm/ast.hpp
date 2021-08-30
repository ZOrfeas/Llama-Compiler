#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <iomanip>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include "symbol.hpp"
#include "types.hpp"
#include "infer.hpp"
 
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

void printColorString(std::string s, int width, int format = 0, int color = 37); 

/* TypeGraphs of basic types useful for type checking ***************/

extern TypeGraph *type_unit;
extern TypeGraph *type_int;
extern TypeGraph *type_float;
extern TypeGraph *type_bool;
extern TypeGraph *type_char;

/********************************************************************/

class Identifier
{
protected:
    std::string id;
    TypeGraph *TG;
    int line;
public:
    Identifier(std::string id, int line)
     : id(id), line(line) { TG = st.lookup(id)->getTypeGraph(); }
    std::string getName() 
    {
        return id;
    }
    std::string getTypeString() 
    {
        TypeGraph *correctTypeGraph = inf.deepSubstitute(TG);
        return correctTypeGraph->stringifyTypeClean();
    }
    std::string getLine()
    {
        return std::to_string(line);
    }
    void printIdLine(int lineWidth, int idWidth, int typeWidth)  
    {
        printColorString(getName(), idWidth, 1, 35);
        printColorString(getTypeString(), typeWidth);
        printColorString(getLine(), lineWidth);
        std::cout << std::endl;
    }
};

extern std::vector<Identifier *> AST_identifier_list;

/********************************************************************/

class AST
{
protected:
    int line_number;
    static llvm::LLVMContext TheContext;
    static llvm::IRBuilder<> Builder;
    static llvm::Module *TheModule;
    static llvm::legacy::FunctionPassManager *TheFPM;

    static llvm::Type *i1;
    static llvm::Type *i8;
    static llvm::Type *i32;
    static llvm::Type *flt;
    static llvm::Type *unitType;
    // static llvm::Type *str;

    static llvm::ConstantInt* c1(bool b);
    static llvm::ConstantInt* c8(char c);
    static llvm::ConstantInt* c32(int n);
    static llvm::ConstantFP* f64(double d);
    static llvm::Constant* unitVal();

public:
    AST()
    {
        line_number = yylineno;
    }
    virtual ~AST() {}
    virtual void printOn(std::ostream &out) const = 0;
    virtual void sem() {}
    virtual llvm::Value* compile() { return nullptr; }
    void start_compilation(const char *programName, bool optimize=false);
    virtual void checkTypeGraphs(TypeGraph *t1, TypeGraph *t2, std::string msg)
    {
        if (!t1->isUnknown() && !t2->isUnknown())
        {
            if (!t1->equals(t2))
            {
                printError(msg);
            }
        }
        else
        {
            inf.addConstraint(t1, t2, line_number);
        }
    }
    virtual void printError(std::string msg)
    {   
        std::string intro = "Error at line " + std::to_string(line_number) + ": ";
        printColorString(intro, intro.size(), 1, 31);
        std::cout << msg << std::endl;
        exit(1);
    }
    virtual void insertBasicToSymbolTable(std::string id, TypeGraph *t) 
    {
        st.insertBasic(id, t);
    }
    virtual void insertRefToSymbolTable(std::string id, TypeGraph *t) 
    {   
        st.insertRef(id, t);   
    }
    virtual void insertArrayToSymbolTable(std::string id, TypeGraph *contained_type, int d) 
    {
        st.insertArray(id, contained_type, d);
    }
    virtual FunctionEntry *insertFunctionToSymbolTable(std::string id, TypeGraph *t) 
    {
        return st.insertFunction(id, t);
    }
    virtual void insertTypeToTypeTable(std::string id) 
    {
        if(!tt.insertType(id))
        {
            printError("Type " + id + " has already been defined");
        }
    }
    virtual ConstructorEntry *insertConstructorToConstructorTable(std::string Id)
    {
        ConstructorEntry *c = ct.insertConstructor(Id);
        if(!c) 
        {
            std::string type = lookupConstructorFromContstructorTable(Id)->getTypeGraph()->getCustomType()->stringifyTypeClean();
            printError("Constructor " + Id + " already belongs to type " + type);
        }
        
        return c;
    }
    virtual SymbolEntry *lookupBasicFromSymbolTable(std::string id) 
    {
        SymbolEntry *s = st.lookup(id, false);
        if(!s) 
        {
            printError("Identifier " + id + " not found");
        }

        return s;
    }
    virtual ArrayEntry *lookupArrayFromSymbolTable(std::string id)
    {
        ArrayEntry *s = st.lookupArray(id, false);
        if(!s) 
        {
            printError("Array identifier " + id + " not found");
        }
        
        return s;
    }
    virtual TypeEntry *lookupTypeFromTypeTable(std::string id)
    {
        TypeEntry *t = tt.lookupType(id, false);
        if(!t) 
        {
            printError("Type " + id + " not found");
        }

        return t;
    }
    virtual ConstructorEntry *lookupConstructorFromContstructorTable(std::string Id) 
    {
        ConstructorEntry *c = ct.lookupConstructor(Id, false);
        if(!c)
        {
            printError("Constructor " + Id + " not found");
        }
        
        return c;
    }
    virtual void printIdTypeGraphs() 
    {
        // Find the maximum size of the ids, type and line strings
        // in order to decide the padding
        int margin = 2;
        int idWidth = 4, typeWidth = 8, lineWidth = 3;
        std::string name, typeString, line;
        for(auto ident: AST_identifier_list)
        {
            name = ident->getName();
            typeString = ident->getTypeString();
            line = ident->getLine();

            if(idWidth < (int)name.size()) idWidth = name.size();
            if(typeWidth < (int)typeString.size()) typeWidth = typeString.size();
            if(lineWidth < (int)line.size()) lineWidth = line.size();
        }
        idWidth += margin;
        typeWidth += margin;
        lineWidth += margin;

        // Print the header of the table
        int colorANSI = 34 , formatANSI = 4;
        printColorString("Name", idWidth, formatANSI, colorANSI);
        printColorString("Type", typeWidth, formatANSI, colorANSI);
        printColorString("Line", lineWidth, formatANSI, colorANSI);
        std::cout << std::endl;

        // Print every line
        for (auto ident: AST_identifier_list)
        {
            ident->printIdLine(lineWidth, idWidth, typeWidth);
        }
    }
    virtual void addToIdList(std::string id) 
    {
        AST_identifier_list.push_back(new Identifier(id, line_number));
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
    category c;              // Shows what kind of object it is
    TypeGraph *TG = nullptr; // If it is nullptr it hasn't got a value yet

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
public:
    UnknownType()
        : Type(category::CATEGORY_unknown) { TG = new UnknownTypeGraph(true, true, false); }
    virtual TypeGraph *get_TypeGraph() override
    {
        return TG;
    }
    virtual void printOn(std::ostream &out) const override
    {
        out << "UnknownType(" << TG->stringifyType() << ")";
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
        if (!TG)
            TG = lookupTypeFromTypeTable(type_string[(int)t])->getTypeGraph();

        return TG;
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
        if (!TG)
            TG = new ArrayTypeGraph(dimensions, elem_type->get_TypeGraph());

        return TG;
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
    RefType(Type *ref_type = new UnknownType())
        : Type(category::CATEGORY_ref), ref_type(ref_type) {}
    virtual TypeGraph *get_TypeGraph() override
    {
        if (!TG)
            TG = new RefTypeGraph(ref_type->get_TypeGraph());

        return TG;
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
        if (!TG)
            TG = lookupTypeFromTypeTable(id)->getTypeGraph();

        return TG;
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
    void type_check(TypeGraph *t, std::string msg = "Type mismatch")
    {
        checkTypeGraphs(TG, t, msg + ", " + TG->stringifyTypeClean() + " given.");
    }
    void checkIntCharFloat(std::string msg = "Must be int, char or float")
    {
        if(!TG->isUnknown()) 
        {
            if(!TG->equals(type_int) && !TG->equals(type_char) && !TG->equals(type_float))
            {
                printError(msg);
            }
        }
        else
        {
            TG->setIntCharFloat();
        }
    }
    friend void same_type(Expr *e1, Expr *e2, std::string msg = "Type mismatch")
    {
        e1->type_check(e2->TG, msg);
    }
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
        ConstructorEntry *c = insertConstructorToConstructorTable(Id);
        for (Type *t : type_list)
        {
            c->addType(t->get_TypeGraph());
        }

        te->addConstructor(c);
    }
    virtual llvm::Value* compile() override;
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
        insertBasicToSymbolTable(id, T->get_TypeGraph());

        addToIdList(id);
    }
    TypeGraph *get_TypeGraph()
    {
        return T->get_TypeGraph();
    }
    std::string getId() {return id;}
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
        insertTypeToTypeTable(id);
    }
    virtual void sem() override
    {
        TypeEntry *t = lookupTypeFromTypeTable(id);

        for (Constr *c : constr_list)
        {
            c->add_Id_to_ct(t);
        }
    }
    virtual llvm::Value* compile() override;
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
        insertBasicToSymbolTable(id, T->get_TypeGraph());

        addToIdList(id);
    }
    // - if it is a true Constant definition stores the result of the expr->codegen()
    // - Danger if it's a copy of an already existing function or other edge cases
    virtual llvm::Value* compile() override;
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
        FunctionEntry *F = insertFunctionToSymbolTable(id, T->get_TypeGraph());

        // Add the parameters to the entry
        for (Par *p : par_list)
        {
            F->addParam(p->get_TypeGraph());
        }

        addToIdList(id);
    }
    // - Generates the function prototype
    // - creates a scope, inserts the paramete names and values
    // - calls expr->codegen()
    virtual llvm::Value* compile() override;
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
    Mutable(std::string id, Type *T = new UnknownType)
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
            e->sem();
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
        TypeGraph *t = T->get_TypeGraph();
        RefTypeGraph *contained_type = new RefTypeGraph(t);

        if(!t->isUnknown())
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
    // creates a *Value of a struct type. One such type is created/exists for every
    // arrayType (contents and dimensions only differentiate them),
    // these struct types contain a pointer to the contained type,
    // and the length of every dimension (could be done as independent fields, or in an array)
    virtual llvm::Value* compile() override;
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
        out << "], " << *T << ")";
    }
};
class Variable : public Mutable
{
public:
    Variable(std::string *id, Type *T = new UnknownType)
        : Mutable(*id, T) {}
    virtual void insert_id_to_st() override
    {
        TypeGraph *t = T->get_TypeGraph();
        TypeGraph *ref_type = new RefTypeGraph(t);

        if(!t->isUnknown()) 
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
    // alloca's the necessary space for a var of its TYPE
    virtual llvm::Value* compile() override;
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
    // in order compile the definitions contained 
    // (recursive or not is irrelevant for functions if prototypes are defined at the start)
    virtual llvm::Value* compile() override;
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
    // in order create the types defined 
    // (Struct for big type, with pointers to constr types and enum ?)
    virtual llvm::Value* compile() override;
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
    // in order compile all the contained definitions
    virtual llvm::Value* compile() override;
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
    // open scope, do the definition, compile the expression, return its result Value*
    virtual llvm::Value* compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "LetIN(" << *letdef << ", " << *expr << ")";
    }
};

/*
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
*/

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
        TG = new ArrayTypeGraph(1, new RefTypeGraph(type_char));
    }
    // generate a char array constant(?) and return its Value*
    virtual llvm::Value* compile() override;
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
    // Generate a char constant (byte) and return its Value*
    // possibly llvm accepts escape sequences as they are so we may need to reconstruct them
    virtual llvm::Value* compile() override;
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
    // Generate a bool (bit?) constant and return its Value*
    virtual llvm::Value* compile() override;
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
    // generate a float constant and return its Value*
    virtual llvm::Value* compile() override;
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
    // generate an int constant (check size) and return its Value*
    virtual llvm::Value* compile() override;
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
    // There is something called empty struct type {} in llvm
    // (http://nondot.org/~sabre/LLVMNotes/EliminatingVoid.txt#:~:text=The%20'void'%20type%20in%20llvm,return%20value%20of%20a%20function.&text=In%20the%20LLVM%20IR%2C%20instead,'%3A%20the%20empty%20struct%20type.)
    // could be what we want to use
    virtual llvm::Value* compile() override;
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
    // switch-case for every possible operator
    virtual llvm::Value* compile() override;
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
    // switch-case for every possible operator
    virtual llvm::Value* compile() override;
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

        TG = new RefTypeGraph(t);
    }
    // malloc's a new spot in memory and returns its value (probably :) )
    virtual llvm::Value* compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "New(" << *new_type << ")";
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
        cond->sem();
        body->sem();
        // Typecheck
        cond->type_check(type_bool, "While condition must be bool");
        body->type_check(type_unit, "While body must be unit");

        TG = type_unit;
    }
    // compiles the loop, take note of the condition.
    // phi node may be necessary, avoidable if we can be sure
    // that the condition is "constant" (pointer dereference, or some shit)
    virtual llvm::Value* compile() override;
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
    // could possibly alloc a variable to use for the loop
    virtual llvm::Value* compile() override;
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
    // again think about phi nodes, otherwise its a simple if compilation,
    // noteworthy: no 'else' means else branch just jumps to end
    virtual llvm::Value* compile() override;
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
        ArrayEntry *arr = lookupArrayFromSymbolTable(id);

        // Get the number of the dimension
        int i = dim->get_int();
        // std::cout << i << "\n";
        // std::cout << arr->getTypeGraph()->getDimensions() << "\n";
        // Check if i is withing the correct bounds
        if (i < 1 && i > arr->getTypeGraph()->getDimensions())
        {
            printError("Index out of bounds");
        }

        TG = type_int;
    }
    // llvm may have our backs, may store some runtime (or at least the expression)
    // info about the length of an array (through its type system)
    virtual llvm::Value* compile() override;
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
        SymbolEntry *s = lookupBasicFromSymbolTable(id);
        TG = s->getTypeGraph();
    }
    // lookup and return the Value* stored, special case if it's a function
    virtual llvm::Value* compile() override;
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
                expr_list[i]->type_check(correct_t, err + std::to_string(i + 1));
            }

            TG = definitionTypeGraph->getResultType();
        }
        else
        {
            printError(id + " already declared as non-function");
        }
    }
    // get the function prototype and call it, return the Value* of the call
    virtual llvm::Value* compile() override;
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
        ConstructorEntry *c = lookupConstructorFromContstructorTable(Id);
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
            expr_list[i]->type_check(correct_t, err + std::to_string(i + 1));
        }

        TG = c->getTypeGraph();
    }
    // creates a struct (emplaces it in the big struct sets the enum?)
    virtual llvm::Value* compile() override;
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
        int args_n = (int)expr_list.size();
        SymbolEntry *a = lookupBasicFromSymbolTable(id);
        TypeGraph *t = a->getTypeGraph();

        // If it is known check that the dimensions are correct
        // and the indices provided are integers
        if (!t->isUnknown())
        {
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
    // since inside the struct a simple array is contained,
    // perform the calculation of the actual address before dereferencing
    // (We could if we wanted to, check bounds at runtime and exit with error code)
    virtual llvm::Value* compile() override;
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
        literal->sem();
        checkTypeGraphs(t, literal->get_TypeGraph(), "Literal is not a valid pattern for given type");
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
        insertBasicToSymbolTable(id, t);

        addToIdList(id);
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
        ConstructorEntry *c = lookupConstructorFromContstructorTable(Id);
        TypeGraph *c_TypeGraph = c->getTypeGraph();

        // Check that toMatch is of the same type as constructor or force it to be
        checkTypeGraphs(t, c_TypeGraph, "Constructor is not of the same type as the expression to match");

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
    // Could be implemented kinda like an if-else, very simply in fact for 
    // cases without custom types. 
    // For custom types check the enum to match the constructor call each time,
    // if it matches dereference once and check the inner values recursively
    virtual llvm::Value* compile() override;
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
        TG = curr;
    }
    // generate code for each clause, return the value of its result
    virtual llvm::Value* compile() override;
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
