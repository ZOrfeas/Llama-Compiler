#include "symbol.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

/*************************************************************/
/** Common aliases */
/*************************************************************/
using tScope = std::map<std::string, SymbolEntry *>;
using tTable = std::vector<tScope *>;
using string = std::string;

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
    Table = new tTable();
    if (openGlobalScope) {
        Table->push_back(new tScope());
    }
}

SymbolTable::~SymbolTable() {
    while(closeScope())
        ; // do nothing while deleting scopes
    delete Table; // delete the Table at the end
}

bool SymbolTable::insert(SymbolEntry *entry, bool overwrite = true) {
    if (nameInScope(entry->name, Table->back()) && !overwrite) {
            return false;
    } else {
        // creates or updates existing
        (*Table->back())[entry->name] = entry;
        return true;
    }
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

SymbolEntry* SymbolTable::lookup(string name, EntryType type,
                                 bool err = true) {
    for (auto it = Table->rbegin(); it != Table->rend(); ++it) {
        if (nameInScope(name, *it) && (**it)[name]->eType == type) {
            return (**it)[name];
        }
    }
    std::cout << "SymbolTable:" << std::endl;
    std::cout << "Symbol " << name << " not found" << std::endl;
    if (err) { exit(1); }
    return nullptr;
}

/*************************************************************/
/** SymbolEntry method implementations */
/*************************************************************/

VariableEntry::VariableEntry(std::string n, type *t)
    : SymbolEntry(n,t,EntryType::VARIABLE) {};

ParameterEntry::ParameterEntry(std::string n, type *t)
    : SymbolEntry(n,t,EntryType::PARAMETER) {};

FunctionEntry::FunctionEntry(std::string n, type *t)
    :SymbolEntry(n,t,EntryType::FUNCTION),
    parameters(new std::vector<ParameterEntry *>()) {};
FunctionEntry::~FunctionEntry() { delete parameters; }

ConstantEntry::ConstantEntry(std::string n, type *t)
    : SymbolEntry(n,t,EntryType::CONSTANT) {};

TypeEntry::TypeEntry(std::string n, type *t)
    :SymbolEntry(n,t,EntryType::TYPE),
    constructors(new std::vector<ConstructorEntry *>()) {};
TypeEntry::~TypeEntry() { delete constructors; }

ConstructorEntry::ConstructorEntry(std::string n, type *t)
: SymbolEntry(n,t,EntryType::CONSTRUCTOR) {};


void ParameterEntry::setFunction(FunctionEntry *f)
    { funcEntry = f; }
void FunctionEntry::addParam(ParameterEntry *param)
    { parameters->push_back(param); }
void TypeEntry::addConstructor(ConstructorEntry *constr)
    { constructors->push_back(constr); }
void ConstructorEntry::setType(TypeEntry *t)
    { typeEntry = t; }
