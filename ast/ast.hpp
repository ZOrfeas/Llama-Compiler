#pragma once

#include <iostream>
#include <map>
#include <vector>

class AST {
public:
    virtual ~AST() {}
    virtual void printOn(std::ostream &out) const = 0;
};

inline std::ostream& operator<< (std::ostream &out, const AST &t) {
  t.printOn(out);
  return out;
}

class DefStmt: public AST {

};

class Tdef: public DefStmt {
private:
    std::string id;

};

class Def: public DefStmt {
};

class Function: public Def {

};

class Mutable: public Def {

};

class Definition: public AST {
    virtual void append(DefStmt *d) = 0;
};

class Letdef: public Definition {
private:
    bool recursive;
    std::vector<DefStmt *> def_list;
public:
    Letdef(bool rec = false): recursive(rec), def_list() {}
    void append(DefStmt *d){
        def_list.push_back(d);
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
    Typedef(): tdef_list() {}
    void append(DefStmt *t){
        tdef_list.push_back(t);
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

class Expr: public AST {
public:
    //virtual void eval() const = 0;
};