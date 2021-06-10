#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <map>

enum class type { TYPE_unknown, TYPE_unit, TYPE_int, TYPE_float, TYPE_bool,
            TYPE_string, TYPE_char, TYPE_ref, TYPE_array, TYPE_function };


/*************************************************************/

/** Enum class with availabel SymbolEntry Types */
enum class EntryType {
    VARIABLE, FUNCTION, PARAMETER,
    CONSTANT, CONSTRUCTOR, TYPE,
};

/** Parent class for all possible Symbol Entry types */
class SymbolEntry {
public:
    std::string name;
    EntryType eType;
    type *ty;
    SymbolEntry(std::string n, type *t): name(n), ty(t) {};
    virtual ~SymbolEntry() {}
};
/*************************************************************/
// These are necessary to allow circular dependencies
class FunctionEntry;
class ParameterEntry;

class ConstructorEntry;
class TypeEntry;

class VariableEntry: public SymbolEntry {
public:
    VariableEntry(std::string n, type *t);
    ~VariableEntry();
};
class ParameterEntry: public SymbolEntry {
public:
    FunctionEntry *funcEntry;
    void setFunction(FunctionEntry *f);
    ParameterEntry(std::string n, type *t);
    ~ParameterEntry();
};
class FunctionEntry: public SymbolEntry {
public:
    std::vector<ParameterEntry *> *parameters;
    void addParam(ParameterEntry *param);
    FunctionEntry(std::string n, type *t);
    ~FunctionEntry();
};
class ConstantEntry: public SymbolEntry {
public:
    ConstantEntry(std::string n, type *t);
    ~ConstantEntry();
};
class TypeEntry: public SymbolEntry {
public:
    std::vector<ConstructorEntry *> *constructors;
    void addConstructor(ConstructorEntry *constr);
    TypeEntry(std::string n, type *t);
    ~TypeEntry();
};
class ConstructorEntry: public SymbolEntry {
public:
    TypeEntry *typeEntry;
    void setType(TypeEntry *t);
    ConstructorEntry(std::string n, type *t);
    ~ConstructorEntry();
};
/*************************************************************/

/** Symbol Table class */
class SymbolTable {
    /** Pointer to a stack of pointers to maps */
    std::vector<std::map<std::string, SymbolEntry *> *> *Table;
public:
    /** Constructor can opt out of creating the first scope */
    SymbolTable(bool openGlobalScope = true);
    /** Inserts a new symbol entry in the current active scope */
    bool insert(SymbolEntry *entry, bool overwrite = true);
    /** Opens a new scope */
    void openScope();
    /** Closes the currently active scope and discards it */
    bool closeScope(bool deleteEntries = true);
    /** Lookups up the existence of an identifier as a Symbol Entry of the provided EntryType   
     *  If not found and err = true, exits, otherwise returns nullptr */
    SymbolEntry* lookup(std::string name, EntryType type,
                        bool err = true);
    ~SymbolTable();
};
/*************************************************************/