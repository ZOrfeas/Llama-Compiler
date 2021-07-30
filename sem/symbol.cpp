#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "symbol.hpp"

/*************************************************************/
/**                   Common aliases                         */
/*************************************************************/
using tScope = std::map<std::string, SymbolEntry *>;
using sTable = std::vector<tScope *>;
using std::string;

/*************************************************************/
/**                  Helper functions                        */
/*************************************************************/

/** Helper function checking existsence of an identifier in a scope */
bool nameInScope(string name, tScope *scope) {
    return (scope->find(name) != scope->end());
}
/*************************************************************/
/**           SymbolTable method implementations             */
/*************************************************************/

SymbolTable::SymbolTable(bool debug, bool openGlobalScope) {
    this->debug = debug;
    Table = new sTable();
    if (openGlobalScope) {
        openScope();
    }
}
SymbolTable::~SymbolTable() {
    while(closeScope())
        ; // do nothing while deleting scopes
    delete Table; // delete the Table at the end
}
void SymbolTable::error(string msg, bool crash) {
    std::cout << "SymbolTable:" << " ";
    std::cout << msg << std::endl;
    if (crash) exit(1);

}
void SymbolTable::log(string msg) {error(msg, false);}

SymbolEntry* SymbolTable::insert(SymbolEntry *entry, bool overwrite) {
    if (debug) 
        log("Inserting " + entry->getTypeGraph()->stringifyType() + " with name " + entry->name);
    if (nameInScope(entry->name, Table->back()) && !overwrite) {
            return nullptr;
    } else {
        // creates or updates existing
        (*Table->back())[entry->name] = entry;
        return entry;
    }
}
SymbolEntry* SymbolTable::lookup(string name, bool err) {
    if (debug)
        log("Looking up name:" + name);
    for (auto it = Table->rbegin(); it != Table->rend(); ++it) {
        if (nameInScope(name, *it)) {
            return (**it)[name];
        }
    }
    error("Symbol " + name + " not found", err);
    return nullptr;
}

void SymbolTable::openScope() {
    if (debug)
        log("Opening a new scope");
    Table->push_back(new tScope());
}
bool SymbolTable::closeScope(bool deleteEntries) {
    if (debug)
        log("Closing a scope");
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

/******************************************/
/** Insert wrapper method implementations */
/******************************************/

SymbolEntry* SymbolTable::insertBasic(string name, TypeGraph *t, bool overwrite) {
    SymbolEntry* basicEntry = new SymbolEntry(name, t);
    return insert(basicEntry, overwrite);
}
FunctionEntry* SymbolTable::insertFunction(string name, TypeGraph *resT, bool overwrite) {
    FunctionTypeGraph* funcType = new FunctionTypeGraph(resT);
    FunctionEntry* funcEntry = new FunctionEntry(name, funcType);
    return dynamic_cast<FunctionEntry *>(insert(funcEntry, overwrite));
}
ArrayEntry* SymbolTable::insertArray(string name, TypeGraph *containedT, int dimensions,
                                      bool overwrite) {
    ArrayTypeGraph* arrType = new ArrayTypeGraph(dimensions, containedT);
    ArrayEntry* arrEntry = new ArrayEntry(name, arrType);
    return dynamic_cast<ArrayEntry *>(insert(arrEntry, overwrite));
}
RefEntry* SymbolTable::insertRef(string name, TypeGraph *pointedT,
                                    bool allocated, bool dynamic, bool overwrite) {
    RefTypeGraph* refType = new RefTypeGraph(pointedT, allocated, dynamic);
    RefEntry* refEntry = new RefEntry(name,refType);
    return dynamic_cast<RefEntry *>(insert(refEntry, overwrite));
}

/******************************************/
/** Lookup wrapper method implementations */
/******************************************/

FunctionEntry* SymbolTable::lookupFunction(string name, bool err) {
    if (SymbolEntry* candidate = lookup(name, err)) {
        if (candidate->getTypeGraph()->isFunction()) 
            return dynamic_cast<FunctionEntry *>(candidate);
    }
    error("Function " + name + " not found", err);
    return nullptr;
}
ArrayEntry* SymbolTable::lookupArray(string name, bool err) {
    if (SymbolEntry* candidate = lookup(name, err)) {
        if (candidate->getTypeGraph()->isArray()) 
            return dynamic_cast<ArrayEntry *>(candidate);
    }
    error("Array " + name + " not found", err);
    return nullptr;
}
RefEntry* SymbolTable::lookupRef(string name, bool err) {
    if (SymbolEntry* candidate = lookup(name, err)) {
        if (candidate->getTypeGraph()->isRef())
            return dynamic_cast<RefEntry *>(candidate);
    } else
    error("Ref " + name + " not found", err);
    return nullptr;
}

/*************************************************************/
/**           BaseTable method implementations               */
/*************************************************************/

BaseTable::BaseTable(string kind, bool debug) {
    this->debug = debug;
    this->kind = kind;
    Table = new tScope();
}
BaseTable::~BaseTable() {
    if (debug)
        log("destructor called...");
    for (auto &pair: *Table) {
        delete pair.second;
    }
    delete Table;
}
void BaseTable::error(string msg, bool crash) {
    std::cout << kind << ":" <<" ";
    std::cout << msg << std::endl;
    if (crash) exit(1);
}
void BaseTable::log(string msg) {error(msg, false);}

SymbolEntry* BaseTable::insert(SymbolEntry *entry, bool overwrite) {
    if (debug)
        log("Inserting " + entry->getTypeGraph()->stringifyType() + " with name " + entry->name);
    if (nameInScope(entry->name, Table) && !overwrite) {
        string msg = "Name" + entry->name + "declared twice";
        error(msg);
        return nullptr;
    } else {
        // creates or updates existing
        (*Table)[entry->name] = entry;
        return entry;
    }
}
SymbolEntry* BaseTable::lookup(string name, bool err) {
    if (debug)
        log("Looking up name: " + name);
    if (nameInScope(name, Table)) {
        return (*Table)[name];
    }
    error("Name " + name + " not found", err);
    return nullptr;
}

/*************************************************************/
/** TypeTable method implementations */
/*************************************************************/

TypeTable::TypeTable(bool debug): BaseTable("TypeTable", debug) {
    insert(new TypeEntry("int",  new IntTypeGraph()));
    insert(new TypeEntry("float", new FloatTypeGraph()));
    insert(new TypeEntry("char", new CharTypeGraph()));
    insert(new TypeEntry("unit" , new UnitTypeGraph()));
    insert(new TypeEntry("bool" , new BoolTypeGraph()));
}
TypeEntry* TypeTable::insertType(string name, bool overwrite) {
    CustomTypeGraph *customType = new CustomTypeGraph();
    TypeEntry *typeEntry = new TypeEntry(name, customType);
    return dynamic_cast<TypeEntry *>(insert(typeEntry, overwrite));
}
TypeEntry* TypeTable::lookupType(string name, bool err) {
    return dynamic_cast<TypeEntry *>(lookup(name, err));
}
TypeTable::~TypeTable() {}


/*************************************************************/
/** ConstructorTable method implementations */
/*************************************************************/

ConstructorTable::ConstructorTable(bool debug)
: BaseTable("ConstructorTable", debug) {}
ConstructorEntry* ConstructorTable::insertConstructor(string name, bool overwrite) {
    ConstructorTypeGraph *constrType = new ConstructorTypeGraph();
    ConstructorEntry *constructorEntry = new ConstructorEntry(name, constrType);
    return dynamic_cast<ConstructorEntry *>(insert(constructorEntry, overwrite));
}
ConstructorEntry* ConstructorTable::lookupConstructor(string name, bool err) {
    return dynamic_cast<ConstructorEntry *>(lookup(name, err));
}
ConstructorTable::~ConstructorTable() {}

/*************************************************************/
/** SymbolEntry method implementations */
/*************************************************************/
TypeGraph* SymbolEntry::getTypeGraph() { return typeGraph; }
SymbolEntry::SymbolEntry(std::string n, TypeGraph *t)
: name(n), typeGraph(t) {};

FunctionEntry::FunctionEntry(std::string n, TypeGraph *t)
: SymbolEntry(n, t) {}

// FunctionTypeGraph* FunctionEntry::getTypeGraph() {
//     if (typeGraph->isFunction()) 
//         return dynamic_cast<FunctionTypeGraph *>(typeGraph);
//     std::cout << "Entry " << name << " is not associated with a function"
//               << std::endl;
//     exit(1);
// }
void FunctionEntry::addParam(TypeGraph *param, bool push_back) {
    getTypeGraph()->addParam(param, push_back);
}
ArrayEntry::ArrayEntry(std::string n, TypeGraph *t)
: SymbolEntry(n, t) {}

// ArrayTypeGraph* ArrayEntry::getTypeGraph() {
//     if (typeGraph->isArray())
//         return dynamic_cast<ArrayTypeGraph *>(typeGraph);
//     std::cout << "Entry " << name << " is not associated with an array"
//               << std::endl;
//     exit(1);
// }

RefEntry::RefEntry(std::string n, TypeGraph *t)
: SymbolEntry(n, t) {}
// RefTypeGraph* RefEntry::getTypeGraph() {
//     if (typeGraph->isRef())
//         return dynamic_cast<RefTypeGraph *>(typeGraph);
//     std::cout << "Entry " << name << " is not associated with a ref"
//               << std::endl;
//     exit(1);
// }
bool RefEntry::isDynamic() { return getTypeGraph()->isDynamic(); }
bool RefEntry::isAllocated() { return getTypeGraph()->isAllocated(); }
void RefEntry::setDynamic() { getTypeGraph()->setDynamic(); }
void RefEntry::setAllocated() { getTypeGraph()->setAllocated(); }
void RefEntry::resetDynamic() { getTypeGraph()->resetDynamic(); }
void RefEntry::resetAllocated() { getTypeGraph()->resetAllocated(); }


TypeEntry::TypeEntry(std::string n, TypeGraph *t)
    :SymbolEntry(n,t),
    constructors(new std::vector<ConstructorEntry *>()) {}
TypeEntry::~TypeEntry() { 
    for (auto &constructor: *constructors) 
        delete constructor; 
    delete constructors;
}
void TypeEntry::addConstructor(ConstructorEntry *constr) {    
    constructors->push_back(constr);
    constr->setTypeEntry(this);
    getTypeGraph()->addConstructor(
        dynamic_cast<ConstructorTypeGraph*>(constr->getTypeGraph())
    );
}
// CustomTypeGraph* TypeEntry::getTypeGraph() {
//     return dynamic_cast<CustomTypeGraph *>(typeGraph);
// }

ConstructorEntry::ConstructorEntry(std::string n, TypeGraph *t)
    : SymbolEntry(n,t) {};
void ConstructorEntry::setTypeEntry(TypeEntry *t)
    { typeEntry = t; }
// ConstructorTypeGraph* ConstructorEntry::getTypeGraph() {
//     return dynamic_cast<ConstructorTypeGraph *>(typeGraph);
// }
void ConstructorEntry::addType(TypeGraph *field) {
    getTypeGraph()->addField(field);
}

bool             debug_logs = true;
SymbolTable      st(debug_logs);
TypeTable        tt(debug_logs);
ConstructorTable ct(debug_logs);