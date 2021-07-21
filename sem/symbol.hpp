#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <map>

enum class type { TYPE_unknown, TYPE_unit, TYPE_int, TYPE_float, TYPE_bool,
            TYPE_string, TYPE_char, TYPE_ref, TYPE_array, TYPE_function, TYPE_custom };


/*************************************************************/

/** Enum class with available SymbolEntry Types */
enum class EntryType {
    FUNCTION, CONSTANT, 
    CONSTRUCTOR, TYPE,
};

/*************************************************************/
/**                    SymbolEntry                           */
/*************************************************************/
/** Parent class for all possible Symbol Entry types */
class SymbolEntry {
public:
    // Id associated with SymbolEntry
    std::string name;
    // Tracker to be able to Downcast to appropriate subclass
    EntryType eType;
    // (Placeholder) Will hold pointer to Type Graph of entry
    type *ty;
    SymbolEntry(std::string n, type *t, EntryType eType): name(n), ty(t) {};
    virtual ~SymbolEntry() {}
};

/*************************************************************/
/**                SymbolEntry SubClasses                    */
/*************************************************************/
// These are necessary to allow circular dependencies
class ConstructorEntry;
class TypeEntry;

class FunctionEntry: public SymbolEntry {
public:
    // SymbolEntries of Function params
    std::vector<ConstantEntry *> *parameters; 
    // Adds a function param to this function (doesn't add to SymbolTable)
    void addParam(ConstantEntry *param);
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
    // SymbolEntries of Constructors
    std::vector<ConstructorEntry *> *constructors;
    // Adds a constructor to this type (doesn't add to TypeTable)
    void addConstructor(ConstructorEntry *constr);
    TypeEntry(std::string n, type *t);
    ~TypeEntry();
};
class ConstructorEntry: public SymbolEntry {
public:
    // The TypeEntry this constructor corresponds to
    TypeEntry *typeEntry;
    // Sets this Constructor's TypeEntry
    void setType(TypeEntry *t);
    ConstructorEntry(std::string n, type *t);
    ~ConstructorEntry();
};
/*************************************************************/
/**                SymbolTable class                         */
/*************************************************************/
class SymbolTable {
    /** Pointer to a stack of pointers to maps */
    std::vector<std::map<std::string, SymbolEntry *> *> *Table;
    /** Looks up the existence of an identifier as a Symbol Entry of the provided EntryType   
     *  If not found and err = true, exits, otherwise returns nullptr */
    SymbolEntry* lookup(std::string name, EntryType type,
                        bool err = true);
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

    // Insert wrappers
    /** insert wrapper for ConstantEntries */
    ConstantEntry* insertConstant(std::string name, type *t, bool overwrite = true);
    /** insert wrapper for FunctionEntries */
    FunctionEntry* insertFunction(std::string name, type *t, bool overwrite = true);
    // Lookup wrappers
    /** lookup wrapper for ConstantEntries */
    ConstantEntry* lookupConstant(std::string name, bool err = true);
    /** lookup wrapper for FunctionEntries */
    FunctionEntry* lookupFunction(std::string name, bool err = true);

    ~SymbolTable();
};

/*************************************************************/
/**                TypeTable class                           */
/*************************************************************/
class TypeTable {
    /** Pointer to a map of name-Entry_pointer pairs*/
    std::map<std::string, SymbolEntry *> *Table;
    SymbolEntry *lookup(std::string name, EntryType type,
                        bool err = true);
    SymbolEntry* insert(SymbolEntry *entry, bool overwrite = false);
    void error(std::string msg, bool crash = true);
    void log(std::string msg);
    bool debug;
public:
    TypeTable(bool debug = false);
    // Insert wrappers
    /** insert wrapper for TypeEntries */
    TypeEntry* insertType(std::string name, type *t, bool overwrite = false);
    /** insert wrapper for ConstructorEntries */
    ConstructorEntry* insertConstructor(std::string, type *t, bool overwrite = false);
    // Lookup wrappers
    /** lookup wrapper for TypeEntries */
    TypeEntry* lookupType(std::string name, bool err = true);
    /** lookup wrapper for ConstructorEntries */
    ConstructorEntry* lookupConstructor(std::string name, bool err = true);
    ~TypeTable();
};

/*************************************************************/
/**                Table instantiations                      */
/*************************************************************/
SymbolTable *st = new SymbolTable();
TypeTable *tt = new TypeTable();