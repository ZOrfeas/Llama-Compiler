#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include <iostream>
#include <vector>
#include <string>

enum class graphType { TYPE_unknown, TYPE_unit, TYPE_int, TYPE_float, TYPE_bool,
            TYPE_char, TYPE_ref, TYPE_array, TYPE_function, TYPE_custom, TYPE_record };

const std::string graph_type_string[] = { "TYPE_unknown", "TYPE_unit", "TYPE_int", "TYPE_float", "TYPE_bool",
                                    "TYPE_char", "TYPE_ref", "TYPE_array", "TYPE_function", "TYPE_record" };



/** Base Type Graph class from which all others are derived */
class TypeGraph {
    graphType t;
public:
    TypeGraph(graphType t): t(t) {}
    std::string stringifyType() {
        return type_string[static_cast<int>(t)];
    }
    void log(std::string msg) {
        std::cout << "TypeGraph of type " << stringifyType()
                  << " says:" << std::endl << msg << std::endl;
    }
    bool isFunction()    { return t == graphType::TYPE_function; }
    bool isArray()       { return t == graphType::TYPE_array;    }
    bool isRef()         { return t == graphType::TYPE_ref;      }
    bool isInt()         { return t == graphType::TYPE_int;      }
    bool isUnit()        { return t == graphType::TYPE_unit;     }
    bool isBool()        { return t == graphType::TYPE_bool;     }
    bool isChar()        { return t == graphType::TYPE_char;     }
    bool isFloat()       { return t == graphType::TYPE_float;    }
    bool isCustom()      { return t == graphType::TYPE_custom;   }
    bool isConstructor() { return t == graphType::TYPE_record;   }
    bool isUnknown()     { return t == graphType::TYPE_unknown;  }
    bool isBasic() { return dynamic_cast<BasicTypeGraph *>(this); }
    bool isDeletable() { return isFunction() || isArray() || isRef(); }
    virtual ~TypeGraph() {}
};
/************************************************************/
class UnknownTypeGraph : public TypeGraph {
public:
    UnknownTypeGraph(): TypeGraph(graphType::TYPE_unknown) {}
    ~UnknownTypeGraph() {}
};
/************************************************************/
/** Base Basic Type Graph class from whic all basic type are derived */
class BasicTypeGraph : public TypeGraph {
public:
    BasicTypeGraph(graphType t): TypeGraph(t) {}
};
class UnitTypeGraph : public BasicTypeGraph {
public:
    UnitTypeGraph() : BasicTypeGraph(graphType::TYPE_unit) {}
    ~UnitTypeGraph() {}
};
class IntTypeGraph : public BasicTypeGraph {
public:
    IntTypeGraph() : BasicTypeGraph(graphType::TYPE_int) {}
    ~IntTypeGraph() {}
};
class CharTypeGraph : public BasicTypeGraph {
public:
    CharTypeGraph() : BasicTypeGraph(graphType::TYPE_char) {}
    ~CharTypeGraph() {}
};
class BoolTypeGraph : public BasicTypeGraph {
public:
    BoolTypeGraph() : BasicTypeGraph(graphType::TYPE_bool) {}
    ~BoolTypeGraph() {}
};
class FloatTypeGraph : public BasicTypeGraph {
public:
    FloatTypeGraph() : BasicTypeGraph(graphType::TYPE_float) {}
    ~FloatTypeGraph() {}
};
/** Complex Type Graphs */
/************************************************************/
class ArrayTypeGraph : public TypeGraph {
    TypeGraph *Type;
public:
    int dimensions;
    ArrayTypeGraph(int dimensions, TypeGraph *containedType)
    : TypeGraph(graphType::TYPE_array), dimensions(dimensions), Type(containedType) {}
    TypeGraph* getContainedType() { return Type; }
    ~ArrayTypeGraph() { if (Type->isDeletable()) delete Type; }
};
class RefTypeGraph : public TypeGraph {
    TypeGraph *Type;
public:
    bool allocated, dynamic;
    RefTypeGraph(TypeGraph *refType, bool allocated = false, bool dynamic = false)
    : TypeGraph(graphType::TYPE_ref), Type(refType), allocated(allocated), dynamic(dynamic) {}
    TypeGraph* getContainedType() { return Type; }
    void setAllocated() { allocated = true; }
    void setDynamic() { dynamic = true; }
    void resetAllocated() { allocated = false; }
    void resetDynamic() { dynamic = false; }
    ~RefTypeGraph() { if (Type->isDeletable()) delete Type; }
};
class FunctionTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *paramTypes; 
    TypeGraph *resultType;
public:
    FunctionTypeGraph(TypeGraph *resultType)
    : TypeGraph(graphType::TYPE_function), resultType(resultType), paramTypes(new std::vector<TypeGraph *>()) {}
    std::vector<TypeGraph *>* getParamTypes() { return paramTypes; }
    TypeGraph* getResultType() { return resultType; }
    int getParamCount() { return paramTypes->size(); }
    /** Utility method for creating more complex FunctionTypeGraphs */
    void addParam(TypeGraph *param) { paramTypes->push_back(param); }
    TypeGraph* getParamType(int index) {
        if (index >= paramTypes->size() || index < 0) {
            std::cout << "Out of bounds param requested";
            exit(1);
        }
        return (*paramTypes)[index];
    }
    ~FunctionTypeGraph() {
        for (auto &paramType: *paramTypes) 
            if (paramType->isDeletable()) delete paramType;
        if (resultType->isDeletable()) delete resultType;;
    }
};
/** This represents a singular constructor */
class ConstructorTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *fields;
    CustomTypeGraph *customType;
public:
    ConstructorTypeGraph(std::vector<TypeGraph *> *fields, CustomTypeGraph *cType)
    : TypeGraph(graphType::TYPE_record), fields(fields), customType(cType) {}
    std::vector<TypeGraph *>* getFields() { return fields; }
    void setTypeGraph(CustomTypeGraph *owningType) { customType = owningType; }
    int getFieldCount() { return fields->size(); }
    TypeGraph* getFieldType(int index) {
        if (index >= fields->size() || index < 0) {
            std::cout << "Out of bounds param requested";
            exit(1);
        }
        return (*fields)[index];
    }
    ~ConstructorTypeGraph() { for (auto &field: *fields) if (field->isDeletable()) delete field; }
};
class CustomTypeGraph : public TypeGraph {
    std::vector<ConstructorTypeGraph *> *constructors;
public:
    CustomTypeGraph(std::vector<ConstructorTypeGraph *> *constructors = new std::vector<ConstructorTypeGraph *>())
    : TypeGraph(graphType::TYPE_custom), constructors(constructors) {}
    std::vector<ConstructorTypeGraph *>* getConstructors() { return constructors; }
    int getConstructorCount() { return constructors->size(); }
    void addConstructor(ConstructorTypeGraph *constructor) {
        constructors->push_back(constructor);
        constructor->setTypeGraph(this);
    }
    ~CustomTypeGraph() { for (auto &constructor: *constructors) delete constructor; }
};

/** Global basic graphType classes instantiation */

UnitTypeGraph unitType;
IntTypeGraph intType;
CharTypeGraph charType;
BoolTypeGraph boolType;
FloatTypeGraph floatType;

/** Utility functions */

#endif