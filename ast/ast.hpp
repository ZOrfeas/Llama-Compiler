#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "symbol.hpp"
const std::string type_string[] = { "TYPE_unknown", "TYPE_unit", "TYPE_int", "TYPE_float", "TYPE_bool",
                                    "TYPE_string", "TYPE_char", "TYPE_ref", "TYPE_arrray", "TYPE_function" };

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
class Type: public AST {};

class BasicType: public Type {
private:
    type t;
public:
    BasicType(type t): t(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << type_string[t];
    }
};

class FunctionType: public Type {
private:
    Type *lhtype, *rhtype;
public:
    FunctionType(Type *lhtype, Type *rhtype): lhtype(lhtype), rhtype(rhtype) {}
    virtual void printOn(std::ostream &out) const override {
        out << *lhtype << "->" << *rhtype;
    }
};

class ArrayType: public Type {
private:
    int dimensions;
    Type *type;
public:
    ArrayType(int dimensions, Type *type): dimensions(dimensions), type(type) {}
    virtual void printOn(std::ostream &out) const override {
        out << "TYPE_array(" << dimensions << ", " << *type << ")";
    }
};

/* NOTE: MUST NOT BE ArrayType !! */
class RefType: public Type {
private:
    Type *type;
public:
    RefType(Type *type): type(type) {}
    virtual void printOn(std::ostream &out) const override {
        out << "TYPE_ref(" << *type << ")";
    }
};

class CustomType: public Type {};

// Basic abstract classes --------------------------------------------
class Expr: public AST {
protected:
    Type *type;
public:
    void type_check(Type *t) {
        sem();
        if (type != t) yyerror("Type mismatch");
    }
};

class Identifier: public Expr {
protected:
    std::string name;
};

//--------------------------------------------------------------------
class Constr: public AST {
private:
    std::string Id;
    std::vector<Type *> type_list;
public:
    Constr(std::string *Id, std::vector<Type *> *t): Id(*Id), type_list(*t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Constr(" << Id;

        for(Type *t: type_list){
            out << ", " << t;
        }

        out << ")";
    }
};

//--------------------------------------------------------------------
class Par: public AST {
private:
    std::string id;
    Type *type;
public:
    Par(std::string *id, Type *t = new BasicType(TYPE_unknown)): id(*id), type(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Par(" << id << ", " << *type << ")";
    }
};

//--------------------------------------------------------------------
class DefStmt: public AST {
};

class Tdef: public DefStmt {
private:
    std::string id;
    std::vector<Constr *> constr_list;
public:
    Tdef(std::string *id, std::vector<Constr *> *c): id(*id), constr_list(*c) {}
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
};

class Function: public Def {
private:
    std::string id;
    std::vector<Par *> par_list;
    Type *type;
    Expr *expr;
public:
    Function(std::string *id, std::vector<Par *> *p, Expr *e, Type *t = new BasicType(TYPE_unknown)): id(*id), par_list(*p), type(t), expr(e) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Function(" << id;
        for(Par *p: par_list){ out << ", " << *p; }
        out << ", " << *type << ", " << *expr << ")";
    }
};

class Mutable: public Def {
private:
    std::string id;
    std::vector<Expr *> expr_list;
    Type *type;
public:
    Mutable(std::string *id, std::vector<Expr *> *e, Type *t = new BasicType(TYPE_unknown)): id(*id), expr_list(*e), type(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Mutable(" << id;
        if(!expr_list.empty()) {
            out << "[";

            bool first = true;
            for(Expr *e: expr_list){
                if(!first) out << ", ";
                else first = false;

                out << *e;
            }
        }
        out << ", " << *type << ")";
    }
};

class Variable: public Def {
private:
    std::string id;
    Expr *expr;
    Type *type;
public:
    Variable(std::string *id, Expr *expr, Type *type = new BasicType(TYPE_unknown)): id(*id), expr(expr), type(type) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Variable(" << id << ", " << *type << ", " << *expr << ")";
    }
};

//--------------------------------------------------------------------
class Definition: public AST {
    // virtual void append(DefStmt *d) = 0;
};

class Letdef: public Definition {
private:
    bool recursive;
    std::vector<DefStmt *> def_list;
public:
    Letdef(std::vector<DefStmt *> *d, bool rec = false): recursive(rec), def_list(*d) {}
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

//--------------------------------------------------------------------
class Program: public AST {
private:
    std::vector<Definition *> definition_list;
public:
    ~Program() {}
    Program(): definition_list() {}
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

// Exresssions -------------------------------------------------------
// class Expr: public AST (used to be here)

// class Identifier: public Expr (used to be here)

class LetIn: public Expr {
private:
    Definition *letdef;
    Expr *expr;
public:
    LetIn(Definition *letdef, Expr *expr): letdef(letdef), expr(expr) {}
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

class Const: public Expr {

};

class String_literal: public Const {
private:
    std::string s;
public:
    String_literal(std::string *s): s(*s) { type = new BasicType(TYPE_string); }
    virtual void printOn(std::ostream &out) const override {
        out << "String(" << s << ")";
    }
};

class Char_literal: public Const {
private:
    std::string c_string;
    char c;
public:
    Char_literal(std::string *c_string): c_string(*c_string) { 
        type = new BasicType(TYPE_char); 
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

class Bool_literal: public Const {
private:
    bool b;
public:
    Bool_literal(bool b): b(b) { type = new BasicType(TYPE_bool); }
    virtual void printOn(std::ostream &out) const override {
        out << "Bool(" << b << ")";
    }
};

class Float_literal: public Const {
private:
    double d;
public:
    Float_literal(double d): d(d) { type = new BasicType(TYPE_float); }
    virtual void printOn(std::ostream &out) const override {
        out << "Float(" << d << ")";
    }
};

class Int_literal: public Const {
private:
    int n;
public:
    Int_literal(int n): n(n) { type = new BasicType(TYPE_int); }
    virtual void printOn(std::ostream &out) const override {
        out << "Int(" << n << ")";
    }
};

class Unit: public Const {
public:
    Unit() { type = new BasicType(TYPE_unit); }
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
    virtual void printOn(std::ostream &out) const override {
        out << "UnOp(" << op << ", " << *expr << ")";
    }
};

class While: public Expr {
private:
    Expr *cond, *body;
public:
    While(Expr *e1, Expr *e2): cond(e1), body(e2) { type = new BasicType(TYPE_unit); }
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
    For(std::string *id, Expr *e1, std::string s, Expr *e2, Expr *e3): id(*id), step(s), start(e1), finish(e2), body(e3) { type = new BasicType(TYPE_unit); }
    virtual void printOn(std::ostream &out) const override {
        out << "For(" << id << ", " << *start << ", " << step << *finish << ", " << *body << ")";
    }
};

class If: public Expr{
private:
    Expr *cond, *body, *else_body;
public:
    If(Expr *e1, Expr *e2, Expr *e3 = nullptr): cond(e1), body(e2), else_body(e3) { type = new BasicType(TYPE_unit); }
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
    virtual void printOn(std::ostream &out) const override {
        out << "Dim(";
        if(dim) out << *dim << ", ";
        out << id << ")";
    }
};

class FunctionCall: public Expr {
private:
    std::string id;
    std::vector<Expr *> expr_list;
public:
    FunctionCall(std::string *id, std::vector<Expr *> *expr_list): id(*id), expr_list(*expr_list) {}
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
    ConstructorCall(std::string *Id, std::vector<Expr *> *expr_list): Id(*Id), expr_list(*expr_list) {}
    virtual void printOn(std::ostream &out) const override {
        out << "ConstructorCall(" << Id;
        for(Expr *e: expr_list) {
            out << ", " << *e;
        }
        out << ")";
    }
};