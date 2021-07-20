#include "symbol.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

/*************************************************************/
/** Common aliases */
/*************************************************************/
using tScope = std::map<std::string, SymbolEntry *>;
using sTable = std::vector<tScope *>;
using std::string;

/*************************************************************/
/** Helper functions */
/*************************************************************/

/** Helper function checking existsence of an identifier in a scope */
bool nameInScope(string name, tScope *scope) {
    return (scope->find(name) != scope->end());
}

/*************************************************************/
/** SymbolTable method implementations */
/*************************************************************/

SymbolTable::SymbolTable(bool openGlobalScope = true) {
    Table = new sTable();
    if (openGlobalScope) {
        Table->push_back(new tScope());
    }
}
SymbolTable::~SymbolTable() {
    while(closeScope())
        ; // do nothing while deleting scopes
    delete Table; // delete the Table at the end
}
void SymbolTable::error(string msg, bool crash = true) {
    std::cout << "SymbolTable:" << std::endl;
    std::cout << msg << std::endl;
    if (crash) exit(1);

}

SymbolEntry* SymbolTable::insert(SymbolEntry *entry, bool overwrite = true) {
    if (nameInScope(entry->name, Table->back()) && !overwrite) {
            return nullptr;
    } else {
        // creates or updates existing
        (*Table->back())[entry->name] = entry;
        return entry;
    }
}
SymbolEntry* SymbolTable::lookup(string name, EntryType type,
                                 bool err = true) {
    for (auto it = Table->rbegin(); it != Table->rend(); ++it) {
        if (nameInScope(name, *it) && (**it)[name]->eType == type) {
            return (**it)[name];
        }
    }
    error("Symbol " + name + " not found", err);
    return nullptr;
}

void SymbolTable::openScope() {
    Table->push_back(new tScope());
}
bool SymbolTable::closeScope(bool deleteEntries = true) {
    if (Table->size() != 0) {
        tScope *poppedScope = Table->back();
        Table->pop_back();
        if (deleteEntries) {
            for (auto &pair: *poppedScope) {
                delete pair.second;
            }
        }
        delete poppedScope;
        return true;
    }
    return false;
}

VariableEntry* SymbolTable::insertVariable(string name, type *t, bool dynamic, bool overwrite = true) {
    VariableEntry *ve = new VariableEntry(name, dynamic, t);
    insert(ve, overwrite);
    return ve;
}
ConstantEntry* SymbolTable::insertConstant(string name, type *t, bool overwrite = true) {
    ConstantEntry *ce = new ConstantEntry(name, t);
    insert(ce, overwrite);
    return ce;
}
FunctionEntry* SymbolTable::insertFunction(string name, type *t, bool overwrite = true) {
    FunctionEntry *fe = new FunctionEntry(name, t);
    insert(fe, overwrite);
    return fe;
}

VariableEntry* SymbolTable::lookupVariable(string name, bool err = true) {
    SymbolEntry *e = lookup(name, EntryType::VARIABLE, err);
    if (VariableEntry *ve = dynamic_cast<VariableEntry *>(e)) {
        return ve;
    } else {
        error("Internal error. Downcast to requested SymbolEntry subclass failed...");
    }
}
ConstantEntry* SymbolTable::lookupConstant(string name, bool err = true) {
    SymbolEntry *e = lookup(name, EntryType::CONSTANT, err);
    if (ConstantEntry *ce = dynamic_cast<ConstantEntry *>(e)) {
        return ce;
    } else {
        error("Internal error. Downcast to requested SymbolEntry subclass failed...");
    }
}
FunctionEntry* SymbolTable::lookupFunction(string name, bool err = true) {
    SymbolEntry *e = lookup(name, EntryType::FUNCTION, err);
    if (FunctionEntry *fe = dynamic_cast<FunctionEntry *>(e)) {
        return fe;
    } else {
        error("Internal error. Downcast to requested SymbolEntry subclass failed...");
    }
}


/*************************************************************/
/** TypeTable method implementations */
/*************************************************************/

TypeTable::TypeTable() {
    Table = new tScope();
}
TypeTable::~TypeTable() {
    for (auto &pair: *Table) {
        delete pair.second;
    }
    delete Table;
}
void TypeTable::error(string msg, bool crash = true) {
    std::cout << "TypeTable:" << std::endl;
    std::cout << msg << std::endl;
    if (crash) exit(1);
}

SymbolEntry* TypeTable::lookup(string name, EntryType type,
                    bool err = true) {
    if (nameInScope(name, Table) && (*Table)[name]->eType == type) {
        return (*Table)[name];
    }
}
SymbolEntry* TypeTable::insert(SymbolEntry *entry, bool overwrite = false) {
    if (nameInScope(entry->name, Table) && !overwrite) {
        string msg;
        if (std::isupper(entry->name[0])) 
            msg = "Constructor with name ";
        else
            msg =  "Type with name ";
        msg += entry->name + " declared twice";
        error(msg);
    } else {
        // creates or updates existing
        (*Table)[entry->name] = entry;
        return entry;
    }
}

TypeEntry* TypeTable::insertType(string name, type *t, bool overwrite = false) {
    TypeEntry* te = new TypeEntry(name, t);
    insert(te, overwrite);
    return te;
}
ConstructorEntry* TypeTable::insertConstructor(string name, type *t, bool overwrite = false) {
    ConstructorEntry* ce = new ConstructorEntry(name, t);
    insert(ce, overwrite);
    return ce;
}

TypeEntry* TypeTable::lookupType(string name, bool err = true) {
    SymbolEntry *e = lookup(name, EntryType::TYPE, err);
    if (TypeEntry *te = dynamic_cast<TypeEntry *>(e)) {
        return te;
    } else {
        error("Internal error. Downcast to requested SymbolEntry subclass failed...");
    }
}
ConstructorEntry* TypeTable::lookupConstructor(string name, bool err = true) {
    SymbolEntry *e = lookup(name, EntryType::CONSTRUCTOR, err);
    if (ConstructorEntry *ce = dynamic_cast<ConstructorEntry *>(e)) {
        return ce;
    } else {
        error("Internal error. Downcast to requested SymbolEntry subclass failed...");
    }
}


/*************************************************************/
/** SymbolEntry method implementations */
/*************************************************************/

VariableEntry::VariableEntry(std::string n, bool dyn, type *t)
    : SymbolEntry(n,t,EntryType::VARIABLE), dynamic(dyn) {};

FunctionEntry::FunctionEntry(std::string n, type *t)
    :SymbolEntry(n,t,EntryType::FUNCTION),
    parameters(new std::vector<ConstantEntry *>()) {};
FunctionEntry::~FunctionEntry() { delete parameters; }

ConstantEntry::ConstantEntry(std::string n, type *t)
    : SymbolEntry(n,t,EntryType::CONSTANT) {};

TypeEntry::TypeEntry(std::string n, type *t)
    :SymbolEntry(n,t,EntryType::TYPE),
    constructors(new std::vector<ConstructorEntry *>()) {};
TypeEntry::~TypeEntry() { delete constructors; }

ConstructorEntry::ConstructorEntry(std::string n, type *t)
: SymbolEntry(n,t,EntryType::CONSTRUCTOR) {};


void FunctionEntry::addParam(ConstantEntry *param)
    { parameters->push_back(param); }
void ConstructorEntry::setType(TypeEntry *t)
    { typeEntry = t; }
void TypeEntry::addConstructor(ConstructorEntry *constr) {
    constructors->push_back(constr);
    constr->setType(this);
}
