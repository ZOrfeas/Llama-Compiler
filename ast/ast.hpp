#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "symbol.hpp"
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

class Expr: public AST {
protected:
    Type type;
public:
    void type_check(Type t) {
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
    Identifier *Id;
    std::vector<Type> type_list;
public:
    Constr(Identifier *Id, std::vector<Type> &t): Id(Id), type_list(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Constr(" << *Id;

        for(Type t: type_list){
            out << ", " << t;
        }

        out << ")";
    }
};

//--------------------------------------------------------------------
class Par: public AST {
private:
    Identifier *id;
    Type type;
public:
    Par(Identifier *id, Type t = TYPE_unknown): id(id), type(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Par(" << *id << ", " << type << ")";
    }
};

//--------------------------------------------------------------------
class DefStmt: public AST {
};

class Tdef: public DefStmt {
private:
    Identifier *id;
    std::vector<Constr *> constr_list;
public:
    Tdef(Identifier *id, std::vector<Constr *> &c): id(id), constr_list(c) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Tdef(" << *id << ", ";

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
    Identifier *id;
    std::vector<Par *> par_list;
    Type type;
    Expr *expr;
public:
    Function(Identifier *id, std::vector<Par *> &p, Expr *e, Type t = TYPE_unknown): id(id), par_list(p), type(t), expr(e) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Def(" << *id;
        for(Par *p: par_list){ out << ", " << *p; }
        out << ", " << type << ", " << expr << ")";
    }
};

class Mutable: public Def {
private:
    Identifier *id;
    std::vector<Expr *> expr_list;
    Type type;
public:
    Mutable(Identifier *id, std::vector<Expr *> &e, Type t = TYPE_unknown): id(id), expr_list(e), type(t) {}
    virtual void printOn(std::ostream &out) const override {
        out << "Def(" << *id;
        if(!expr_list.empty()) {
            out << "[";

            bool first = true;
            for(Expr *e: expr_list){
                if(!first) out << ", ";
                else first = false;

                out << *e;
            }
        }
        out << ", " << type << ")";
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
    Letdef(std::vector<DefStmt *> &d, bool rec = false): recursive(rec), def_list(d) {}
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
    Typedef(std::vector<DefStmt *> &t): tdef_list(t) {}
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

//--------------------------------------------------------------------
// class Expr: public AST (used to be here)

// class Identifier: public Expr (used to be here)

class Id_upper: public Identifier {
public:
    Id_upper(std::string s) { name = s; }
    virtual void printOn(std::ostream &out) const override {
        out << "Id(" << name << ")";
    }
};

class Id_lower: public Identifier {
public:
    Id_lower(std::string s) { name = s; }
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
    String_literal(std::string s): s(s) { type = TYPE_string; }
    virtual void printOn(std::ostream &out) const override {
        out << "String(" << s << ")";
    }
};

class Char_literal: public Const {
private:
    char c;
public:
    Char_literal(char c): c(c) { type = TYPE_char; }
    virtual void printOn(std::ostream &out) {
        out << "Char(" << c << ")";
    }
};

class Bool_literal: public Const {
private:
    bool b;
public:
    Bool_literal(bool b): b(b) { type = TYPE_bool; }
    virtual void printOn(std::ostream &out) const override {
        out << "Bool(" << b << ")";
    }
};

class Float_literal: public Const {
private:
    double d;
public:
    Float_literal(double d): d(d) { type = TYPE_float; }
    virtual void printOn(std::ostream &out) {
        out << "Float(" << d << ")";
    }
};

class Int_literal: public Const {
private:
    int n;
public:
    Int_literal(int n): n(n) { type = TYPE_int; }
    virtual void printOn(std::ostream &out){
        out << "Int(" << n << ")";
    }
};

class Unit: public Const {
public:
    Unit() { type = TYPE_unit; }
    virtual void printOn(std::ostream &out) {
        out << "unit";
    }
};

class BinOp: public Expr {
private:
    Expr *lhs, *rhs;
    int op;
public:
    BinOp(Expr *e1, int op, Expr *e2): lhs(e1), rhs(e2), op(op) {}
};

class UnOp: public Expr {
private:
    Expr *expr;
    int op;
public:
    UnOp(int op, Expr *e): expr(e), op(op) {}
};

class While: public Expr {
private:
    Expr *cond, *body;
public:
    While(Expr *e1, Expr *e2): cond(e1), body(e2) { type = TYPE_unit; }
    virtual void printOn(std::ostream &out){
        out << "While(" << *cond << ", " << *body << ")";
    }
};

class For: public Expr {
private:
    Identifier *id;
    std::string step;
    Expr *start, *finish, *body;
public:
    For(Identifier *id, Expr *e1, std::string s, Expr *e2, Expr *e3): id(id), step(s), start(e1), finish(e2), body(e3) { type = TYPE_unit; }
    virtual void printOn(std::ostream &out){
        out << "For(" << *id << ", " << *start << ", " << step << *finish << ", " << *body << ")";
    }
};

class If: public Expr{
private:
    Expr *cond, *body, *else_body;
public:
    If(Expr *e1, Expr *e2, Expr *e3 = nullptr): cond(e1), body(e2), else_body(e3) { type = TYPE_unit; }
    virtual void printOn(std::ostream &out) {
        out << "If(" << *cond << ", " << *body;
        
        if(else_body) out << ", " << *else_body;
        
        out << ")";
    }
};

class Dim: public Expr {
private:
    Int_literal *i;
    Identifier *id;
public:
    Dim(Identifier *id, Int_literal *i = nullptr): i(i), id(id) {}
    virtual void printOn(std::ostream &out) {
        out << "Dim(";
        
        if(i) out << *i << ", ";

        out << *id << ")";
    }
};
