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
    Identifier(std::string id, int line);
    std::string getName();
    std::string getTypeString();
    std::string getLine();
    void printIdLine(int lineWidth, int idWidth, int typeWidth);
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
    static llvm::Type *machinePtrType;
    static llvm::Type *arrCharType;

    static llvm::ConstantInt *c1(bool b);
    static llvm::ConstantInt *c8(char c);
    static llvm::ConstantInt *c32(int n);
    static llvm::ConstantInt *c64(long int n);
    static llvm::Constant *f80(long double d);
    static llvm::Constant *unitVal();
    std::vector<std::pair<std::string, llvm::Function *>> *genLibGlueLogic();
    static llvm::Function *createFuncAdapterFromUnitToVoid(llvm::Function *unitFunc);
    static llvm::Function *createFuncAdapterFromCharArrToString(llvm::Function *charArrFunc);
    static llvm::Function *createFuncAdapterFromVoidToUnit(llvm::Function *voidFunc);
    static llvm::Function *createFuncAdapterFromStringToCharArr(llvm::Function *stringFunc);

public:
    AST();
    virtual ~AST();
    virtual void printOn(std::ostream &out) const = 0;
    virtual void sem();
    virtual llvm::Value *compile()
    {
        return nullptr;
    }
    void start_compilation(const char *programName, bool optimize = false);
    void printLLVMIR();
    void checkTypeGraphs(TypeGraph *t1, TypeGraph *t2, std::string msg);
    void printError(std::string msg);
    virtual void insertToTable();
    void insertBasicToSymbolTable(std::string id, TypeGraph *t);
    void insertRefToSymbolTable(std::string id, TypeGraph *t);
    void insertArrayToSymbolTable(std::string id, TypeGraph *contained_type, int d);
    FunctionEntry *insertFunctionToSymbolTable(std::string id, TypeGraph *t);
    void insertTypeToTypeTable(std::string id);
    ConstructorEntry *insertConstructorToConstructorTable(std::string Id);
    SymbolEntry *lookupBasicFromSymbolTable(std::string id);
    ArrayEntry *lookupArrayFromSymbolTable(std::string id);
    TypeEntry *lookupTypeFromTypeTable(std::string id);
    ConstructorEntry *lookupConstructorFromContstructorTable(std::string Id);
    void printIdTypeGraphs();
    void addToIdList(std::string id);
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
    Type(category c);
    category get_category();
    bool compare_category(category _c);
    virtual bool compare_basic_type(type t);
    virtual TypeGraph *get_TypeGraph() = 0;
    friend bool compare_categories(Type *T1, Type *T2);
};
class UnknownType : public Type
{
public:
    UnknownType();
    virtual TypeGraph *get_TypeGraph() override;
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
    BasicType(type t);
    virtual bool compare_basic_type(type _t) override;
    virtual TypeGraph *get_TypeGraph() override;
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
    FunctionType(Type *lhtype = new UnknownType, Type *rhtype = new UnknownType);
    virtual TypeGraph *get_TypeGraph() override;
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
    ArrayType(int dimensions = 1, Type *elem_type = new UnknownType);
    virtual TypeGraph *get_TypeGraph() override;
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
    RefType(Type *ref_type = new UnknownType());
    virtual TypeGraph *get_TypeGraph() override;
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
    CustomType(std::string *id);
    virtual TypeGraph *get_TypeGraph() override;
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

public:
    TypeGraph *get_TypeGraph();
    void type_check(TypeGraph *t, std::string msg = "Type mismatch");
    void checkIntCharFloat(std::string msg = "Must be int, char or float");
    friend void same_type(Expr *e1, Expr *e2, std::string msg);
};

/* Useful classes for definitions ***********************************/

class Constr : public AST
{
private:
    std::string Id;
    std::vector<Type *> type_list;

public:
    Constr(std::string *Id, std::vector<Type *> *t);
    void add_Id_to_ct(TypeEntry *te);
    virtual llvm::Value *compile() override;
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

class Par : public AST
{
private:
    std::string id;
    Type *T;

public:
    Par(std::string *id, Type *t = new UnknownType);
    virtual void insertToTable() override;
    TypeGraph *get_TypeGraph();
    std::string getId();
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
    DefStmt(std::string id);
    virtual bool isFunctionDefinition() const;
    virtual void insertToTable();
};
class Tdef : public DefStmt
{
private:
    std::vector<Constr *> constr_list;

public:
    Tdef(std::string *id, std::vector<Constr *> *c);
    virtual void insertToTable() override;
    virtual void sem() override;
    virtual llvm::Value *compile() override;
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
    Def(std::string id, Type *t);
    Type *get_type();
};
class Constant : public Def
{
protected:
    Expr *expr;

public:
    Constant(std::string *id, Expr *e, Type *t = new UnknownType);
    virtual void sem() override;
    virtual void insertToTable() override;
    // - if it is a true Constant definition stores the result of the expr->codegen()
    // - Danger if it's a copy of an already existing function or other edge cases
    virtual llvm::Value *compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "Constant(" << id << ", " << *T << ", " << *expr << ")";
    }
};
class Function : public Constant
{
private:
    std::vector<Par *> par_list;
    TypeGraph *TG;
    llvm::Function *funcPrototype;

public:
    Function(std::string *id, std::vector<Par *> *p, Expr *e, Type *t = new UnknownType);
    virtual void sem() override;
    virtual bool isFunctionDefinition() const override;
    virtual void insertToTable() override;
    // - Generates the function prototype
    // - creates a scope, inserts the paramete names and values
    // - calls expr->codegen()
    void generateLLVMPrototype();
    void generateBody();
    virtual llvm::Value *compile() override;
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
    Mutable(std::string id, Type *T = new UnknownType);
};
class Array : public Mutable
{
private:
    std::vector<Expr *> expr_list;

public:
    Array(std::string *id, std::vector<Expr *> *e, Type *T = new UnknownType);
    virtual void sem() override;
    int get_dimensions();
    virtual void insertToTable();
    // creates a *Value of a struct type. One such type is created/exists for every
    // arrayType (contents and dimensions only differentiate them),
    // these struct types contain a pointer to the contained type,
    // and the length of every dimension (could be done as independent fields, or in an array)
    virtual llvm::Value *compile() override;
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
    Variable(std::string *id, Type *T = new UnknownType);
    virtual void insertToTable() override;
    // alloca's the necessary space for a var of its TYPE
    virtual llvm::Value *compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "Variable(" << id << ", " << *T << ", "
            << ")";
    }
};

/********************************************************************/

class Definition : public AST
{
};
class Letdef : public Definition
{
private:
    bool recursive;
    std::vector<DefStmt *> def_list;

public:
    Letdef(std::vector<DefStmt *> *d, bool rec = false);
    virtual void sem() override;
    // in order compile the definitions contained
    // (recursive or not is irrelevant for functions if prototypes are defined at the start)
    virtual llvm::Value *compile() override;
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
    Typedef(std::vector<DefStmt *> *t);
    virtual void sem() override;
    // in order create the types defined
    // (Struct for big type, with pointers to constr types and enum ?)
    virtual llvm::Value *compile() override;
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
    ~Program();
    Program();
    virtual void sem() override;
    void append(Definition *d);
    // in order compile all the contained definitions
    virtual llvm::Value *compile() override;
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

class LetIn : public Expr
{
private:
    Definition *letdef;
    Expr *expr;

public:
    LetIn(Definition *letdef, Expr *expr);
    virtual void sem() override;
    // open scope, do the definition, compile the expression, return its result Value*
    virtual llvm::Value *compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "LetIN(" << *letdef << ", " << *expr << ")";
    }
};

class Literal : public Expr
{
public:
    virtual llvm::Value *LLVMCompare(llvm::Value *V);
    static char getChar(std::string c);
};
class String_literal : public Literal
{
private:
    std::string s;

public:
    String_literal(std::string *s);
    virtual void sem() override;
    std::string escapeChars(std::string rawStr);
    // generate a char array constant(?) and return its Value*
    virtual llvm::Value *compile() override;
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
    Char_literal(std::string *c_string);
    // Generate a char constant (byte) and return its Value*
    // possibly llvm accepts escape sequences as they are so we may need to reconstruct them
    virtual llvm::Value *compile() override;
    virtual llvm::Value *LLVMCompare(llvm::Value *V) override;
    virtual void sem() override;
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
    Bool_literal(bool b);
    virtual void sem() override;
    // Generate a bool (bit?) constant and return its Value*
    virtual llvm::Value *compile() override;
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
    Float_literal(double d);
    virtual void sem() override;
    // generate a float constant and return its Value*
    virtual llvm::Value *compile() override;
    virtual llvm::Value *LLVMCompare(llvm::Value *V) override;
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
    Int_literal(int n);
    int get_int();
    virtual void sem() override;
    // generate an int constant (check size) and return its Value*
    virtual llvm::Value *compile() override;
    virtual llvm::Value *LLVMCompare(llvm::Value *V) override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "Int(" << n << ")";
    }
};
class Unit_literal : public Literal
{
public:
    Unit_literal();
    virtual void sem() override;
    // There is something called empty struct type {} in llvm
    // (http://nondot.org/~sabre/LLVMNotes/EliminatingVoid.txt#:~:text=The%20'void'%20type%20in%20llvm,return%20value%20of%20a%20function.&text=In%20the%20LLVM%20IR%2C%20instead,'%3A%20the%20empty%20struct%20type.)
    // could be what we want to use
    virtual llvm::Value *compile() override;
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
    BinOp(Expr *e1, int op, Expr *e2);
    virtual void sem() override;
    // switch-case for every possible operator
    virtual llvm::Value *compile() override;
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
    UnOp(int op, Expr *e);
    virtual void sem() override;
    // switch-case for every possible operator
    virtual llvm::Value *compile() override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "UnOp(" << op << ", " << *expr << ")";
    }
};
class New : public Expr
{
    Type *new_type;

public:
    New(Type *t);
    virtual void sem() override;
    // malloc's a new spot in memory and returns its value (probably :) )
    virtual llvm::Value *compile() override;
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
    While(Expr *e1, Expr *e2);
    virtual void sem() override;
    // compiles the loop, take note of the condition.
    // phi node may be necessary, avoidable if we can be sure
    // that the condition is "constant" (pointer dereference, or some shit)
    virtual llvm::Value *compile() override;
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
    For(std::string *id, Expr *e1, std::string s, Expr *e2, Expr *e3);
    virtual void sem() override;
    // could possibly alloc a variable to use for the loop
    virtual llvm::Value *compile() override;
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
    If(Expr *e1, Expr *e2, Expr *e3 = nullptr);
    virtual void sem() override;
    // again think about phi nodes, otherwise its a simple if compilation,
    // noteworthy: no 'else' means else branch just jumps to end
    virtual llvm::Value *compile() override;
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
    Dim(std::string *id, Int_literal *dim = new Int_literal(1));
    virtual void sem() override;
    // llvm may have our backs, may store some runtime (or at least the expression)
    // info about the length of an array (through its type system)
    virtual llvm::Value *compile() override;
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
    ConstantCall(std::string *id);
    virtual void sem() override;
    // lookup and return the Value* stored, special case if it's a function
    virtual llvm::Value *compile() override;
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
    FunctionCall(std::string *id, std::vector<Expr *> *expr_list);
    virtual void sem() override;
    // get the function prototype and call it, return the Value* of the call
    virtual llvm::Value *compile() override;
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
    ConstructorTypeGraph *constructorTypeGraph = nullptr; // Is filled by sem

public:
    ConstructorCall(std::string *Id, std::vector<Expr *> *expr_list = new std::vector<Expr *>());
    virtual void sem() override;
    // creates a struct (emplaces it in the big struct sets the enum?)
    virtual llvm::Value *compile() override;
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
    ArrayAccess(std::string *id, std::vector<Expr *> *expr_list);
    virtual void sem() override;
    // since inside the struct a simple array is contained,
    // perform the calculation of the actual address before dereferencing
    // (We could if we wanted to, check bounds at runtime and exit with error code)
    virtual llvm::Value *compile() override;
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
protected:
    // Will be filled by Clause's and Pattern's compile
    llvm::Value *toMatchV = nullptr;
    llvm::BasicBlock *NextClauseBB = nullptr;

public:
    // Checks whether the pattern is valid for TypeGraph *t
    virtual void checkPatternTypeGraph(TypeGraph *t);
    void set_toMatchV(llvm::Value *v);
    void set_NextClauseBB(llvm::BasicBlock *b)
    {
        NextClauseBB = b;
    }
};
class PatternLiteral : public Pattern
{
protected:
    Literal *literal;

public:
    PatternLiteral(Literal *l);
    virtual void checkPatternTypeGraph(TypeGraph *t) override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternLiteral(" << *literal << ")";
    }
    virtual llvm::Value *compile() override;
};
class PatternId : public Pattern
{
protected:
    std::string id;

public:
    PatternId(std::string *id);
    virtual void checkPatternTypeGraph(TypeGraph *t) override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternId(" << id << ")";
    }
    virtual llvm::Value *compile() override;
};
class PatternConstr : public Pattern
{
protected:
    std::string Id;
    std::vector<Pattern *> pattern_list;

    // Will be filled by checkPatternTypeGraph
    ConstructorTypeGraph *constrTypeGraph;

public:
    PatternConstr(std::string *Id, std::vector<Pattern *> *p_list = new std::vector<Pattern *>());
    virtual void checkPatternTypeGraph(TypeGraph *t) override;
    virtual void printOn(std::ostream &out) const override
    {
        out << "PatternConstr(" << Id;
        for (Pattern *p : pattern_list)
        {
            out << ", " << *p;
        }
        out << ")";
    }
    virtual llvm::Value *compile() override;
};

class Clause : public AST
{
private:
    Pattern *pattern;
    Expr *expr;

    // Will be filled by the Match's sem
    TypeGraph *correctPatternTypeGraph = nullptr;

public:
    Clause(Pattern *p, Expr *e);
    virtual void sem() override;
    void set_correctPatternTypeGraph(TypeGraph *t);
    TypeGraph *get_exprTypeGraph();
    llvm::Value *tryToMatch(llvm::Value *toMatchV, llvm::BasicBlock *NextClauseBB);
    // Could be implemented kinda like an if-else, very simply in fact for
    // cases without custom types.
    // For custom types check the enum to match the constructor call each time,
    // if it matches dereference once and check the inner values recursively
    virtual llvm::Value *compile() override;
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
    Match(Expr *e, std::vector<Clause *> *c);
    virtual void sem() override;
    // generate code for each clause, return the value of its result
    virtual llvm::Value *compile() override;
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
