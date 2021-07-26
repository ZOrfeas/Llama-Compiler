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
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
        if (BasicTypeGraph *b1 = dynamic_cast<BasicTypeGraph*>(this) &&
            BasicTypeGraph *b2 = dynamic_cast<BasicTypeGraph*>(o)) {
            return b1->equals(b2);
        } else if (ArrayTypeGraph *a1 = dynamic_cast<ArrayTypeGraph*>(this) &&
            ArrayTypeGraph *a2 = dynamic_cast<ArrayTypeGraph*>(o)) {
            return a1->equals(a2);
        } else if (RefTypeGraph *r1 = dynamic_cast<RefTypeGraph*>(this) &&
            RefTypeGraph *r2 = dynamic_cast<RefTypeGraph*>(o)) {
            return r1->equals(r2);
        } else if (FunctionTypeGraph *f1 = dynamic_cast<FunctionTypeGraph*>(this) &&
            FunctionTypeGraph *f2 = dynamic_cast<FunctionTypeGraph*>(o)) {
            return f1->equals(f2);
        } else if (ConstructorTypeGraph *c1 = dynamic_cast<ConstructorTypeGraph*>(this) &&
            ConstructorTypeGraph *c2 = dynamic_cast<ConstructorTypeGraph*>(o)) {
            return c1->equals(c2);
        } else if (CustomTypeGraph *t1 = dynamic_cast<CustomTypeGraph*>(this) &&
            CustomTypeGraph *t2 = dynamic_cast<CustomTypeGraph*>(o)) {
            return t1->equals(t2);
        } else {
            log("equals() control reached (imo) unreachable spot");
            exit(1);
        }
    }
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
    virtual bool equals(TypeGraph *o) {
        if (this == o || t == o->t) return true;
        return false;
    }
    ~BasicTypeGraph() {}
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
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
        if (ArrayTypeGraph *tmp = dynamic_cast<ArrayTypeGraph *>(o) &&
            dimensions == tmp->dimensions) {
            return Type->equals(o);
        }
    }
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
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
        if (RefTypeGraph *tmp = dynamic_cast<RefTypeGraph *>(o)) {
            return Type->equals(o);
        }
    }
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
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
        if (FunctionTypeGraph *tmp = dynamic_cast<FunctionTypeGraph *>(o) &&
            getParamCount() == tmp->getParamCount()) {
            for (int i = 0; i < getParamCount(); i++) {
                if (! getParamType(i)->equals(tmp->getParamType(i)) )
                    return false;
            }
            return getResultType()->equals(tmp->getResultType());
        }
    }
    ~FunctionTypeGraph() {
        for (auto &paramType: *paramTypes) 
            if (paramType->isDeletable()) delete paramType;
        if (resultType->isDeletable()) delete resultType;;
    }
};

// forward declarations, possibly avoidable if we decide
// and make sure that constructor equality happens only when
// their customTypeGraphs are the same
class CustomTypeGraph : public TypeGraph {
public:
    virtual bool equals(TypeGraph *o);
};
/** This represents a singular constructor */
class ConstructorTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *fields;
    CustomTypeGraph *customType;
public:
    ConstructorTypeGraph() : TypeGraph(graphType::TYPE_record),
    fields(new std::vector<TypeGraph *>()), customType(nullptr) {}
    std::vector<TypeGraph *>* getFields() { return fields; }
    void addField(TypeGraph *field) { fields->push_back(field); }
    void setTypeGraph(CustomTypeGraph *owningType) { customType = owningType; }
    CustomTypeGraph* getCustomType() { return customType; }
    int getFieldCount() { return fields->size(); }
    TypeGraph* getFieldType(int index) {
        if (index >= fields->size() || index < 0) {
            std::cout << "Out of bounds param requested";
            exit(1);
        }
        return (*fields)[index];
    }
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
        if (ConstructorTypeGraph *tmp = dynamic_cast<ConstructorTypeGraph *>(o)) {
            return getCustomType()->equals(tmp->getCustomType());
        }
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
    virtual bool equals(TypeGraph *o) {
        if (this == o) return true;
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