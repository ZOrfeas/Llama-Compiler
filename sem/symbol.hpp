#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include "types.hpp"

/*************************************************************/
/**                    SymbolEntry                           */
/*************************************************************/
/** Parent class for all possible Symbol Entry types */
class SymbolEntry {
public:
    // Id associated with SymbolEntry
    std::string name;
    // Holds pointer to Type Graph of entry
    TypeGraph *typeGraph;
    virtual TypeGraph* getTypeGraph() { return typeGraph; }
    SymbolEntry(std::string n, TypeGraph *t): name(n), typeGraph(t) {};
    virtual ~SymbolEntry() {}
};

/*************************************************************/
/**                SymbolEntry SubClasses                    */
/*************************************************************/
// These are necessary to allow circular dependencies

class FunctionEntry : public SymbolEntry {
public:
    FunctionEntry(std::string n, TypeGraph *t): SymbolEntry(n, t) {}
    virtual FunctionTypeGraph* getTypeGraph();
    /** Utility method for creating more complex FunctionTypeGraphs 
     * @param push_back If true then param is appended, else inserted at start */
    void addParam(TypeGraph *param, bool push_back = true);
    ~FunctionEntry();
};
class ArrayEntry : public SymbolEntry {
public:
    ArrayEntry(std::string n, TypeGraph *t): SymbolEntry(n, t) {}
    virtual ArrayTypeGraph* getTypeGraph();
    ~ArrayEntry();
};
class RefEntry : public SymbolEntry {
public:
    RefEntry(std::string n, TypeGraph *t): SymbolEntry(n, t) {}
    virtual RefTypeGraph* getTypeGraph();
    bool isDynamic() { return getTypeGraph()->dynamic; }
    bool isAllocated() {return getTypeGraph()->allocated;}
    void setDynamic() { getTypeGraph()->setDynamic(); }
    void setAllocated() { getTypeGraph()->setAllocated(); }
    void resetDynamic() { getTypeGraph()->resetDynamic(); }
    void resetAllocated() { getTypeGraph()->resetAllocated(); }
    ~RefEntry();
};

class TypeEntry: public SymbolEntry {
public:
    // SymbolEntries of Constructors
    std::vector<ConstructorEntry *> *constructors;
    // Adds a constructor to this type (doesn't add to TypeTable)
    void addConstructor(ConstructorEntry *constr);
    virtual CustomTypeGraph* getTypeGraph();
    TypeEntry(std::string n, TypeGraph *t);
    ~TypeEntry();
};
class ConstructorEntry: public SymbolEntry {
public:
    // The TypeEntry this constructor corresponds to
    TypeEntry *typeEntry;
    // Sets this Constructor's TypeEntry
    void setTypeEntry(TypeEntry *t);
    void addType(TypeGraph *field);
    virtual ConstructorTypeGraph* getTypeGraph();
    ConstructorEntry(std::string n, TypeGraph *t);
    ~ConstructorEntry();
};
/*************************************************************/
/**                SymbolTable class                         */
/*************************************************************/
class SymbolTable {
    /** Pointer to a stack of pointers to maps */
    std::vector<std::map<std::string, SymbolEntry *> *> *Table;
    /** Inserts a new symbol entry in the current active scope */
    SymbolEntry* insert(SymbolEntry *entry, bool overwrite = true);
    void error(std::string msg, bool crash = true);
    void log(std::string msg);
    bool debug;
public:
    /** Constructor can opt out of creating the first scope */
    SymbolTable(bool debug = false, bool openGlobalScope = true);
    /** Opens a new scope */
    void openScope();
    /** Closes the currently active scope and discards it */
    bool closeScope(bool deleteEntries = true);
//! Insert wrappers
    /** Inserts an id with the provided TypeGraph as is */
    SymbolEntry* insertBasic(std::string name, TypeGraph *t, bool overwrite = true);
    // SymbolEntry* insertUnknown(std::string name, AST *node, bool overwrite = true);
    /** Inserts an id with the provided TypeGraph as the result of a parameterless FunctionTypeGraph */
    FunctionEntry* insertFunction(std::string name, TypeGraph *resT, bool overwrite = true);
    /** Inserts an id with ArrayTypeGraph of the provided type and dimensions */
    ArrayEntry* insertArray(std::string name, TypeGraph *containedT, int dimensions,
                             bool overwrite = true);
    /** Inserts an id with RefTypeGraph of the provided type and specifications */
    RefEntry* insertRef(std::string name, TypeGraph *pointedT, bool allocated,
                           bool dynamic, bool overwrite = true);
//! Lookup wrappers
    /** Looks up the existence of an identifier 
     *  If not found and err = true, exits, otherwise returns nullptr */
    SymbolEntry* lookup(std::string name, bool err = true);
    /** Looks up a name associated with a callable (function) */
    FunctionEntry* lookupFunction(std::string name, bool err = true);
    /** Looks up a name assocated with an array (dereferencable via '[]') */
    ArrayEntry* lookupArray(std::string name, bool err = true);
    /** Looks up a name associated with a reference type (dereferencable via '!') */
    RefEntry* lookupRef(std::string name, bool err = true);
    ~SymbolTable();
};

/*************************************************************/
/**                Other Table classes                       */
/*************************************************************/
class BaseTable {
    /** Pointer to a map of name-Entry_pointer pairs*/
    std::map<std::string, SymbolEntry *> *Table;
    void error(std::string msg, bool crash = true);
    void log(std::string msg);
    bool debug;
    std::string kind;
public:
    BaseTable(std::string kind = "BaseTable", bool debug = false);
    SymbolEntry *lookup(std::string name, bool err = true);
    SymbolEntry* insert(SymbolEntry *entry, bool overwrite = false);
    virtual ~BaseTable();
};

class TypeTable : public BaseTable {
public:
    TypeTable(bool debug = false): BaseTable("TypeTable", debug) {
        insert(new TypeEntry("int", &intType));
        insert(new TypeEntry("float", &floatType));
        insert(new TypeEntry("char", &charType));
        insert(new TypeEntry("unit" , &unitType));
        insert(new TypeEntry("bool" , &boolType));
    }
        /** insert wrapper for TypeEntries */
    TypeEntry* insertType(std::string name, bool overwrite = false);
    /** lookup wrapper for TypeEntries */
    TypeEntry* lookupType(std::string name, bool err = true);
    ~TypeTable();
};
class ConstructorTable : public BaseTable {
public:
    ConstructorTable(bool debug = false): BaseTable("ConstructorTable", debug) {};
    /** insert wrapper for ConstructorEntries */
    ConstructorEntry* insertConstructor(std::string name, bool overwrite = false);
    /** lookup wrapper for ConstructorEntries */
    ConstructorEntry* lookupConstructor(std::string name, bool err = true);
    ~ConstructorTable();
};


/*************************************************************/
/**                Table instantiations                      */
/*************************************************************/

SymbolTable st;
TypeTable tt;
ConstructorTable ct;

/*************************************************************/
/**              Basic type entry instantiations             */
/**            Bad code, move to main to avoid tmps          */
/*************************************************************/

// moved inside constructor