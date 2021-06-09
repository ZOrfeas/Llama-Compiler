#pragma once

#include <cstdlib>
#include <vector>
#include <string>

enum class type { TYPE_unknown, TYPE_unit, TYPE_int, TYPE_float, TYPE_bool,
            TYPE_string, TYPE_char, TYPE_ref, TYPE_array, TYPE_function };


/*************************************************************/
enum class EntryType {
    VARIABLE, FUNCTION, PARAMETER,
    CONSTANT, CONSTRUCTOR, TYPE,
};

class SymbolEntry {
private:
public:
    EntryType eType;
    virtual ~SymbolEntry() {}
};

class VariableEntry: public SymbolEntry {
public:
    type *t;
    VariableEntry(const char* name, type *t);
};
class ParameterEntry: public SymbolEntry {
public:
    type *t;
    ParameterEntry(const char* name, type *t);
};
class FunctionEntry: public SymbolEntry {
public:
    type *t;
    std::vector<ParameterEntry *> parameters;
    FunctionEntry(const char* name, type *t);
    void addParam(ParameterEntry *);
};
class ConstantEntry: public SymbolEntry {
public:
    type *t;
    ConstantEntry(const char* name, type *t);
};
class TypeEntry: public SymbolEntry {};
class ConstructorEntry: public SymbolEntry {
public:
    TypeEntry *customType;
    ConstructorEntry(const char* name, TypeEntry *customType);
};

class SymbolTable {
private:
public:
    SymbolTable();
    void insert(SymbolEntry * entry);
    void openScope();
    void closeScope();
    SymbolEntry* lookup(const char* name, EntryType type,
                        bool err = true);
};
/*************************************************************/