#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "symbol.hpp"
#include "parser.hpp"

const std::string type_string[] = { "TYPE_unknown", "TYPE_unit", "TYPE_int", "TYPE_float", "TYPE_bool",
                                    "TYPE_string", "TYPE_char", "TYPE_ref", "TYPE_array", "TYPE_function" };

//#include "parser.hpp"

void yyerror(const char *msg);

//--------------------------------------------------------------------
class AST {
public:
    virtual ~AST() {}
    virtual void printOn(std::ostream &out) const = 0;
    virtual void sem() {}
};

inline std::ostream& operator<< (std::ostream &out, const AST &t) {
  t.printOn(out);
  return out;
}

// Type classes ------------------------------------------------------
class Type: public AST {
protected:
    type t;
public:
    Type(type t): t(t) {}
    friend bool shallow_compare_types(Type *T1, Type *T2) { return (T1->t == T2->t); }
    
};
class BasicType: public Type {
public:
    BasicType(type t): Type(t) { /* std::cout << type_string[t]; */ }
    virtual void printOn(std::ostream &out) const override {
        out << type_string[static_cast<int>(t)];
    }
};
class FunctionType: public Type {
private:
    Type *lhtype, *rhtype;
public:
    FunctionType(Type *lhtype = new BasicType(type::TYPE_int), Type *rhtype = new BasicType(type::TYPE_int)): lhtype(lhtype), rhtype(rhtype), Type(type::TYPE_function) {}
    virtual void printOn(std::ostream &out) const override {
        out << *lhtype << "->" << *rhtype;
    }
};
class ArrayType: public Type {
private:
    int dimensions;
    Type *elem_type;
public:
    ArrayType(int dimensions = 0, Type *elem_type = new BasicType(type::TYPE_int)): dimensions(dimensions), elem_type(elem_type), Type(type::TYPE_array) {}
    virtual void printOn(std::ostream &out) const override {
        out << "TYPE_array(" << dimensions << ", " << *elem_type << ")";
    }
};
class RefType: public Type {
/* NOTE: MUST NOT BE ArrayType !! */
private:
    Type *ref_type;
public:
    RefType(Type *ref_type = new BasicType(type::TYPE_int)): ref_type(ref_type), Type(type::TYPE_ref) {}
    virtual void printOn(std::ostream &out) const override {
        out << "TYPE_ref(" << *ref_type << ")";
    }
};
class CustomType: public Type {
private: 
    std::string Id;
public:
    CustomType(std::string *Id): Id(*Id), Type(type::TYPE_custom) {}
    virtual void printOn(std::ostream &out) const override {
        out << "CustomType(" << Id << ")";
    }
};

// Some types that are useful for typechecking
Type *BOOL = new BasicType(type::TYPE_bool);
Type *INT = new BasicType(type::TYPE_int);
Type *FLOAT = new BasicType(type::TYPE_float);
Type *CHAR = new BasicType(type::TYPE_char);
Type *STRING = new BasicType(type::TYPE_string);
Type *REF = new RefType();
Type *ARRAY = new ArrayType();
Type *FUNCTION = new FunctionType();
Type *UNIT = new BasicType(type::TYPE_unit);

/* Basic Abstract Classes *******************************************/

class Expr: public AST {
protected:
    Type *T;
    // dynamic shows whether an expression
    // was created with new
    bool dynamic = false; 
public:
    void type_check(Type *t) {
        sem();
        if (T != t) { 
            yyerror("Type mismatch");
            exit(1);
        }
    }
    /* Succeeds only if T is inside the vector
     * If negation is true then it succeeds only 
     * if T is not in the vector
     */
    void type_check(std::vector< Type* > Type_vect, bool negation = false) {
        sem(); 
        if(!negation) {
            for(Type *temp_T: Type_vect) {
                if(shallow_compare_types(T, temp_T)) return; 
            }
            yyerror("Type mismatch");
            exit(1);
        } 
        else {
            for(Type *temp_T: Type_vect) {
                if(shallow_compare_types(T, temp_T)) {
                    yyerror("Type mismatch");
                    exit(1);
                }
            }
            return;
        }
    }
    friend void same_type(Expr *e1, Expr *e2) {
        e1->sem();
        e2->sem();

        if(e1->T != e2->T) {
            yyerror("Type mismatch");
            exit(1);
        }
    }
    Type* get_type() {
        // NOTE: Careful with this, it returns the pointer
        return T;
    }
    void dynamic_check() { 
        if(!dynamic) {
            yyerror("Not created with new");
            exit(1);
        }
    }
};

class Identifier: public Expr {
protected:
    std::string name;
};

/********************************************************************/

class Constr: public AST {
private:
    std::string Id;
    std::vector<Type *> type_list;
public:
    Constr(std::string *Id, std::vector<Type *> *t): Id(*Id), type_list(*t) {}
    virtual void sem() override {
        tt.insert(Id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Constr(" << Id;

        for(Type *t: type_list){
            out << ", " << *t;
        }

        out << ")";
    }
};

/********************************************************************/

class Par: public AST {
private:
    std::string id;
    Type *T;
public:
    Par(std::string *id, Type *t = new BasicType(type::TYPE_unknown)): id(*id), T(t) {}
    virtual void sem() override {
        st.insert(id);
    }
    Type* get_type() { return T; }
    virtual void printOn(std::ostream &out) const override {
        out << "Par(" << id << ", " << *T << ")";
    }
};

/********************************************************************/

class DefStmt: public AST {
protected:
    std::string id;
public:
    DefStmt(std::string id): id(id) {}
    virtual void insert_id_to_st() {}
    virtual void insert_id_to_tt() {}
};
class Tdef: public DefStmt {
private:
    std::vector<Constr *> constr_list;
public:
    Tdef(std::string *id, std::vector<Constr *> *c): DefStmt(*id), constr_list(*c) {}
    virtual void insert_id_to_tt() override {
        // Insert custom type to type table
        tt.insert(id);
    }
    virtual void sem() override {
        for(Constr *c: constr_list) { c->sem(); }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Tdef(" << id << ", ";

        bool first = true;
        for(Constr *c: constr_list){
            if(!first) out << ", ";
            else first = false;

            out << *c; 
        }

        out << ")";
    }
};
class Def: public DefStmt {
protected: 
    // T is the BasicType of the defined identifier
    Type *T;
public:
    Def(std::string id, Type *t): DefStmt(id), T(t) {}
    Type* get_type() { /* NOTE: Returns the pointer */ return T; }
};
class Constant: public Def {
protected:
    Expr *expr;
public:
    Constant(std::string *id, Expr *e, Type *t = new BasicType(type::TYPE_unknown)): Def(*id, t), expr(e) {}
    virtual void sem() override { expr->type_check(T); }
    virtual void insert_id_to_st() override {
        // Insert constant to symbol table
        st.insert(id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Constant(" << id << ", " << *T << ", " << *expr << ")";
    }
};
class Function: public Constant {
private:
    std::vector<Par *> par_list;
public:
    Function(std::string *id, std::vector<Par *> *p, Expr *e, Type *t = new BasicType(type::TYPE_unknown)): Constant(id, e, t), par_list(*p){}
    virtual void sem() override {
        // Semantically analyse parameters and add them to symbol table
        for(Par *p: par_list) { p->sem(); }

        // Check the type of the expression
        expr->type_check(T);
    }
    virtual void insert_id_to_st() override {
        // Create vector of types of parameters
        std::vector< Type* > par_types;
        for(Par *p: par_list) { par_types.push_back(p->get_type()); }

        // Insert function to symbol table
        st.insert(id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Function(" << id;
        for(Par *p: par_list){ out << ", " << *p; }
        out << ", " << *T << ", " << *expr << ")";
    }
};
class Mutable: public Def {
public:
    Mutable(std::string id, Type *t): Def(id, t) {}
};
class Array: public Mutable {
private:
    std::vector<Expr *> expr_list;
public:
    Array(std::string *id, std::vector<Expr *> *e, Type *t = new BasicType(type::TYPE_unknown)): Mutable(*id, t), expr_list(*e){}
    virtual void sem() override {
        // All dimension sizes are of type integer
        for(Expr *e: expr_list){ e->type_check(INT); }
    }
    int get_dimensions() { return expr_list.size(); }
    virtual void insert_id_to_st() {
        int d = get_dimensions();

        // Insert array to symbol table
        st.insert(id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Array(" << id;
        if(!expr_list.empty()) {
            out << "[";

            bool first = true;
            for(Expr *e: expr_list){
                if(!first) out << ", ";
                else first = false;

                out << *e;
            }
        }
        out << ", " << *T << ")";
    }
};
class Variable: public Mutable {
public:
    Variable(std::string *id, Type *t = new BasicType(type::TYPE_unknown)): Mutable(*id, t) {}
    virtual void insert_id_to_st() override {
        // Add variable to symbol table
        st.insert(id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Variable(" << id << ", " << *T << ", " << ")";
    }
};

/********************************************************************/

class Definition: public AST {
    // virtual void append(DefStmt *d) = 0;
};
class Letdef: public Definition {
private:
    bool recursive;
    std::vector<DefStmt *> def_list;
public:
    Letdef(std::vector<DefStmt *> *d, bool rec = false): recursive(rec), def_list(*d) {}
    virtual void sem() override {
        // Recursive
        if(recursive) {
            // Insert identifiers to symbol table before all the definitions
            for(DefStmt *d: def_list) { d->insert_id_to_st(); }

            // Semantically analyse definitions
            for(DefStmt *d: def_list) { d->sem(); }
        }

        // Not recursive
        else {
            // Semantically analyse definitions
            for(DefStmt *d: def_list) { d->sem(); }

            // Insert identifiers to symbol table after all the definitions
            for(DefStmt *d: def_list) { d->insert_id_to_st(); }
        }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Let(";
        if(recursive) out << "rec ";
        
        bool first = true;
        for(DefStmt *d: def_list){
            if(!first) out << "and ";
            else first = false;

            out << *d;
        }

        out << ")";
    }
};
class Typedef: public Definition {
private:
    std::vector<DefStmt *> tdef_list;
public:
    Typedef(std::vector<DefStmt *> *t): tdef_list(*t) {}
    virtual void sem() override {
        // Insert identifiers to type table before all the definitions
        for(DefStmt *td: tdef_list) { td->insert_id_to_tt(); }

        // Semantically analyse definitions
        for(DefStmt *td: tdef_list) { td->sem(); }
    }
    virtual void printOn(std::ostream &out) const override{
        out << "Type(";

        bool first = true;
        for(DefStmt *t: tdef_list){
            if(!first) out << ", ";
            else first = false;

            out << *t;
        }

        out << ")";
    }
};

/********************************************************************/

class Program: public AST {
private:
    std::vector<Definition *> definition_list;
public:
    ~Program() {}
    Program(): definition_list() {}
    virtual void sem() override {
        for(Definition *d: definition_list) { d->sem(); }
    }
    void append(Definition *d){
        definition_list.push_back(d);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Definition(";
        
        bool first = true;
        for(Definition *d: definition_list){
            if(!first) out << ",";
            else first = false;
            out << *d;
        }

        out << ")";
    }
};

/* Expressions ******************************************************/
// class Expr: public AST (used to be here)
// class Identifier: public Expr (used to be here)

class LetIn: public Expr {
private:
    Definition *letdef;
    Expr *expr;
public:
    LetIn(Definition *letdef, Expr *expr): letdef(letdef), expr(expr) {}
    virtual void sem() override {
        // Create new scope
        st.openScope();

        // Semantically analyse letdef
        letdef->sem();

        // Semantically analyse expression
        expr->sem();

        // Close scope defined by expression
        st.closeScope();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "LetIN(" << *letdef << ", " << *expr << ")";
    }
};

class Id_upper: public Identifier {
public:
    Id_upper(std::string *s) { name = *s; }
    virtual void printOn(std::ostream &out) const override {
        out << "Id(" << name << ")";
    }
};
class Id_lower: public Identifier {
public:
    Id_lower(std::string *s) { name = *s; }
    virtual void printOn(std::ostream &out) const override {
        out << "id(" << name << ")";
    }
};

class Literal: public Expr {};
class String_literal: public Literal {
private:
    std::string s;
public:
    String_literal(std::string *s): s(*s) { T = new BasicType(type::TYPE_string); }
    virtual void printOn(std::ostream &out) const override {
        out << "String(" << s << ")";
    }
};
class Char_literal: public Literal {
private:
    std::string c_string;
    char c;
public:
    Char_literal(std::string *c_string): c_string(*c_string) { 
        T = new BasicType(type::TYPE_char); 
        c = getChar(*c_string);
    }
    
    // Recognise character from string
    char getChar(std::string c) {
        char ans = 0;

        // Normal character
        if(c[1] != '\\'){
            ans =  c[1];
        }

        // '\xnn'
        else if(c[2] == 'x') {
            const char hex[2] = {c[3], c[4]};
            ans = strtol(hex, nullptr, 16);
        }

        // Escape secuence
        else {
            switch(c[2]) {
                case 'n': ans =  '\n'; break;
                case 't': ans = '\t'; break;
                case 'r': ans = '\r'; break;
                case '0': ans = 0; break;
                case '\\': ans = '\\'; break;
                case '\'': ans = '\''; break;
                case '\"': ans = '\"'; break;
            }  
        }

        return ans;
}
    virtual void printOn(std::ostream &out) const override {
        out << "Char(" << c_string << ")";
    }
};
class Bool_literal: public Literal {
private:
    bool b;
public:
    Bool_literal(bool b): b(b) { T = new BasicType(type::TYPE_bool); }
    virtual void printOn(std::ostream &out) const override {
        out << "Bool(" << b << ")";
    }
};
class Float_literal: public Literal {
private:
    double d;
public:
    Float_literal(double d): d(d) { T = new BasicType(type::TYPE_float); }
    virtual void printOn(std::ostream &out) const override {
        out << "Float(" << d << ")";
    }
};
class Int_literal : public Literal
{
private:
    int n;
public:
    Int_literal(int n): n(n) { T = new BasicType(type::TYPE_int); }
    int get_int() { return n; }
    virtual void printOn(std::ostream &out) const override {
        out << "Int(" << n << ")";
    }
};
class Unit_literal: public Literal {
public:
    Unit_literal() { T = new BasicType(type::TYPE_unit); }
    virtual void printOn(std::ostream &out) const override {
        out << "Unit";
    }
};

class BinOp: public Expr {
private:
    Expr *lhs, *rhs;
    int op;
public:
    BinOp(Expr *e1, int op, Expr *e2): lhs(e1), rhs(e2), op(op) {}
    virtual void sem() override {
        switch(op) {
            case '+': case '-': case '*': case '/': case T_mod:
                lhs->type_check(INT);
                rhs->type_check(INT);
                T = new BasicType(type::TYPE_int);
            case T_plusdot: case T_minusdot: case T_stardot: case T_slashdot: case T_dblstar:
                lhs->type_check(FLOAT);
                rhs->type_check(FLOAT);
                T = new BasicType(type::TYPE_float);
            case T_dblbar: case T_dblampersand:
                lhs->type_check(BOOL);
                rhs->type_check(BOOL);
                T = new BasicType(type::TYPE_bool);
            case '=': case T_lessgreater: case T_dbleq: case T_exclameq:
            {   
                // Check that they are not arrays or functions
                std::vector< Type* > v = { ARRAY, FUNCTION };
                lhs->type_check(v, true);
                rhs->type_check(v, true); 

                // Check that they are of the same type
                same_type(lhs, rhs);

                // The result is bool
                T = new BasicType(type::TYPE_bool);
            }
            case '<': case '>': case T_leq: case T_geq:
            {
                // Check that they are char, int or float
                std::vector< Type* > v = { CHAR, INT, FLOAT };
                lhs->type_check(v);
                rhs->type_check(v);

                // Check that they are of the same type
                same_type(lhs, rhs);

                // Get the correct type for the result
                T = lhs->get_type();
            }
            case T_coloneq:
                // Semantically analyse rhs
                rhs->sem();

                // Find the type on the rhs
                Type *rhs_T = rhs->get_type();

                // The lhs must be a ref of the same type as the rhs
                RefType *correct_lhs = new RefType(rhs_T);
                lhs->type_check(correct_lhs);

                // Cleanup
                delete correct_lhs;

                // The result is unit
                T = new BasicType(type::TYPE_unit);
            default: break;
        }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "BinOp(" << *lhs << ", " << op << ", " << *rhs << ")";
    }
};
class UnOp: public Expr {
private:
    Expr *expr;
    int op;
public:
    UnOp(int op, Expr *e): expr(e), op(op) {}
    virtual void sem() override {
        switch(op) {
            case '+': case '-': 
                expr->type_check(INT); 
                T = new BasicType(type::TYPE_int);
            case T_minusdot: case T_plusdot: 
                expr->type_check(FLOAT);
                T = new BasicType(type::TYPE_float);
            case T_not: 
                expr->type_check(BOOL);
                T = new BasicType(type::TYPE_bool);
            case '!': 
                expr->type_check(REF);
                T = expr->get_type();
            case T_delete: 
                expr->type_check(REF);
                expr->dynamic_check();
                T = new BasicType(type::TYPE_unit);
            default: break;
        }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "UnOp(" << op << ", " << *expr << ")";
    }
};
class New: public Expr {
    Type *new_type;
public:
    New(Type *t): new_type(t) {}
    virtual void sem() override {
        // Array type not allowed
        if(shallow_compare_types(new_type, ARRAY)) {
            yyerror("Type mismatch");
            exit(1);
        }

        T = new RefType(new_type); 
        dynamic = true;
    }
    virtual void printOn(std::ostream &out) const override {
        out << "New(" << *T << ")";
    }
};

class While: public Expr {
private:
    Expr *cond, *body;
public:
    While(Expr *e1, Expr *e2): cond(e1), body(e2) { T = new BasicType(type::TYPE_unit); }
    virtual void sem() override {
        // Typecheck
        cond->type_check(BOOL);
        body->type_check(UNIT);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "While(" << *cond << ", " << *body << ")";
    }
};
class For: public Expr {
private:
    std::string id;
    std::string step;
    Expr *start, *finish, *body;
public:
    For(std::string *id, Expr *e1, std::string s, Expr *e2, Expr *e3): id(*id), step(s), start(e1), finish(e2), body(e3) { T = new BasicType(type::TYPE_unit); }
    virtual void sem() override {
        // Create new scope for counter and add it
        st.openScope();
        st.insert(id);

        // Typecheck start, finish, body
        start->type_check(INT);
        finish->type_check(INT);
        body->type_check(UNIT);

        // Close the scope
        st.closeScope();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "For(" << id << ", " << *start << ", " << step << *finish << ", " << *body << ")";
    }
};
class If: public Expr{
private:
    Expr *cond, *body, *else_body;
public:
    If(Expr *e1, Expr *e2, Expr *e3 = nullptr): cond(e1), body(e2), else_body(e3) {}
    virtual void sem() override {
        // Typecheck condition 
        cond->type_check(BOOL);

        // If there is no else just semantically analyse body
        if(else_body == nullptr) { body->sem(); }

        // If there is else then check if the types match
        else { same_type(body, else_body); }

        // The type of the body is the type of the If
        T = body->get_type();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "If(" << *cond << ", " << *body;
        
        if(else_body) out << ", " << *else_body;
        
        out << ")";
    }
};

class Dim: public Expr {
private:
    Int_literal *dim;
    std::string id;
public:
    Dim(std::string *id, Int_literal *dim = new Int_literal(1)): dim(dim), id(*id) {}
    virtual void sem() override {
        // Lookup the array
        Array *arr = st.lookup(id);

        // Get the number of the dimension
        int i = dim->get_int();

        // Check if i is withing the correct bounds
        if(i < 1 && i > arr->get_dimensions()) {
            yyerror("Out of bounds");
            exit(1);
        }

        T = new BasicType(type::TYPE_int);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Dim(";
        if(dim) out << *dim << ", ";
        out << id << ")";
    }
};

// NOTE: All these need lookups
class ConstantCall: public Expr {
protected:
    std::string id;
public:
    ConstantCall(std::string *id): id(*id) {}
    virtual void sem() override {
        // Lookup constant
        Constant *c = st.lookup(id);

        T = c->get_type();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "ConstantCall(" << id << ")";
    }
};
class FunctionCall: public ConstantCall {
private:
    std::vector<Expr *> expr_list;
public:
    FunctionCall(std::string *id, std::vector<Expr *> *expr_list): ConstantCall(id), expr_list(*expr_list) {}
    virtual void printOn(std::ostream &out) const override {
        out << "FunctionCall(" << id;
        for(Expr *e: expr_list) {
            out << ", " << *e;
        }
        out << ")";
    }
};
class ConstructorCall: public Expr {
private:
    std::string Id;
    std::vector<Expr *> expr_list;
public:
    ConstructorCall(std::string *Id, std::vector<Expr *> *expr_list = new std::vector<Expr *>()): Id(*Id), expr_list(*expr_list) {}
    virtual void printOn(std::ostream &out) const override {
        out << "ConstructorCall(" << Id;
        for(Expr *e: expr_list) {
            out << ", " << *e;
        }
        out << ")";
    }
};
class ArrayAccess: public Expr {
private:
    std::string id;
    std::vector<Expr *> expr_list;
public:
    ArrayAccess(std::string *id, std::vector<Expr *> *expr_list): id(*id), expr_list(*expr_list) {}
    virtual void printOn(std::ostream &out) const override {
        out << "ArrayAccess(" << id << "[";
        
        bool first = true;
        for(Expr *e: expr_list){
            if(!first) out << ", ";
            else first = false;

            out << *e;
        }

        out << "])";
    }
};

/* Match ************************************************************/

class Pattern: public AST {};
class PatternLiteral: public Pattern {
protected:
    Literal *literal;
public:
    PatternLiteral(Literal *l): literal(l) {}
    virtual void sem() override {
        Type* T = literal->get_type();
        if(shallow_compare_types(T, UNIT) || shallow_compare_types(T, STRING)) {
            yyerror("Unit and string not allowed in pattern");
            exit(1);
        }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "PatternLiteral(" << *literal << ")";
    }
};
class PatternId: public Pattern {
protected: 
    std::string id;
public:
    PatternId(std::string *id): id(*id) {}
    virtual void sem() override {
        // Create constant with identifier id
        st.insert(id);
    }
    virtual void printOn(std::ostream &out) const override {
        out << "PatternId(" << id << ")";
    }
};
class PatternConstr: public Pattern {
protected:
    std::string Id;
    std::vector< Pattern* > pattern_list;
public:
    PatternConstr(std::string *Id, std::vector< Pattern* > *p_list = new std::vector< Pattern* >()): Id(*Id), pattern_list(*p_list) {}
    virtual void sem() {
        // Lookup constructor
        tt.lookup(Id);

        // Semantically analyse patterns
        for(Pattern *p: pattern_list) { p->sem(); }
    }
    virtual void printOn(std::ostream &out) const override {
        out << "PatternConstr(" << Id;
        for(Pattern *p: pattern_list) { out << ", " << *p; }
        out << ")";
    }
};

class Clause: public AST {
private:
    Pattern *pattern;
    Expr *expr;
public:
    Clause(Pattern *p, Expr *e): pattern(p), expr(e) {}
    virtual void sem() override {
        // Open new scope just in case of PatternId
        st.openScope();
        pattern->sem();
        expr->sem();
        st.closeScope();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Clause(" << *pattern << ", " << *expr << ")";
    }
};
class Match: public Expr {
private:
    Expr *toMatch;
    std::vector<Clause *> clause_list;
public:
    Match(Expr *e, std::vector<Clause *> *c): toMatch(e), clause_list(*c) {}
    virtual void sem() {
        // Semantically analyse expression
        toMatch->sem();

        // Semantically analyse every clause
        for (Clause *c: clause_list) c->sem();
    }
    virtual void printOn(std::ostream &out) const override {
        out << "Match(" << *toMatch;
        for (Clause *c: clause_list) {
            out << ", " << *c;
        }
        out << ")";
    }
};
