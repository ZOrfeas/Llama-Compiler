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
    void wrongCall(std::string functionName) {
        log("Function '" + functionName + "' called for wrong " +
            "subClass, exiting...");
        exit(1);
    }
public:
    TypeGraph(graphType t): t(t) {}
    graphType const & getSubClass() {return t;}
    std::string stringifyType() {
        return graph_type_string[static_cast<int>(t)];
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
    virtual bool equals(TypeGraph *o) = 0;
    virtual TypeGraph* getContainedType() { wrongCall("getContainedType()"); }
    virtual void setDynamic() { wrongCall("setDynamic()"); }
    virtual void setAllocated() { wrongCall("setAllocated"); }
    virtual void resetDynamic()  { wrongCall("resetDynamic"); }
    virtual void resetAllocated() { wrongCall("resetAllocated"); }
    virtual std::vector<TypeGraph *>* getParamTypes() { wrongCall("getParamTypes()"); }
    virtual TypeGraph* getResultType() { wrongCall("getResultType()"); }
    virtual int getParamCount() { wrongCall("getParamCount()"); }
    virtual void addParam(TypeGraph *param, bool push_back = true) { wrongCall("addParam()"); }
    virtual TypeGraph* getParamType(int index) { wrongCall("getParamType"); }
    virtual std::vector<TypeGraph *>* getFields() { wrongCall("getFields()"); }
    virtual void addField(TypeGraph *field) { wrongCall("addField"); }
    virtual void setTypeGraph(CustomTypeGraph *owningType) { wrongCall("setTypeGraph()"); }
    virtual CustomTypeGraph* getCustomType() { wrongCall("getCustomType()"); }
    virtual int getFieldCount() { wrongCall("getFieldCount"); }
    virtual int getConstructorCount() { wrongCall("getConstructorCount()"); }
    virtual void addConstructor(ConstructorTypeGraph *constructor) { wrongCall("addConstructor()"); }
    virtual ~TypeGraph() {}
};
/************************************************************/
class UnknownTypeGraph : public TypeGraph {
public:
    UnknownTypeGraph(): TypeGraph(graphType::TYPE_unknown) {}
    //TODO: not complete
    bool equals(TypeGraph *o) override {}; 
    ~UnknownTypeGraph() {}
};
/************************************************************/
/** Base Basic Type Graph class from whic all basic type are derived */
class BasicTypeGraph : public TypeGraph {
public:
    BasicTypeGraph(graphType t): TypeGraph(t) {}
    bool equals(TypeGraph *o) override {
        if (this == o) return true;
        return getSubClass() == o->getSubClass();
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
    TypeGraph* getContainedType() override { return Type; }
    bool equals(TypeGraph *o) override {
        if (this == o) return true;
        return o->getSubClass() == graphType::TYPE_array &&
               getContainedType()->equals(o->getContainedType());
    }
    ~ArrayTypeGraph() { if (Type->isDeletable()) delete Type; }
};
class RefTypeGraph : public TypeGraph {
    TypeGraph *Type;
public:
    bool allocated, dynamic;
    RefTypeGraph(TypeGraph *refType, bool allocated = false, bool dynamic = false)
    : TypeGraph(graphType::TYPE_ref), Type(refType), allocated(allocated), dynamic(dynamic) {}
    TypeGraph* getContainedType() override { return Type; }
    void setAllocated() override { allocated = true; }
    void setDynamic() override { dynamic = true; }
    void resetAllocated() override { allocated = false; }
    void resetDynamic() override { dynamic = false; }
    bool equals(TypeGraph *o) override {
        if (this == o) return true;
        return o->getSubClass() == graphType::TYPE_ref &&
               getContainedType()->equals(o->getContainedType());
    }
    ~RefTypeGraph() { if (Type->isDeletable()) delete Type; }
};
class FunctionTypeGraph : public TypeGraph {
    std::vector<TypeGraph *> *paramTypes; 
    TypeGraph *resultType;
public:
    FunctionTypeGraph(TypeGraph *resultType)
    : TypeGraph(graphType::TYPE_function), resultType(resultType), paramTypes(new std::vector<TypeGraph *>()) {}
    std::vector<TypeGraph *>* getParamTypes() override { return paramTypes; }
    TypeGraph* getResultType() override { return resultType; }
    int getParamCount() override { return paramTypes->size(); }
    /** Utility method for creating more complex FunctionTypeGraphs 
     * @param push_back If true then param is appended, else inserted at start */
    void addParam(TypeGraph *param, bool push_back = true) override {
        if (push_back)
            paramTypes->push_back(param);
        else
            paramTypes->insert(paramTypes->begin(), param);
    }
    TypeGraph* getParamType(int index) override {
        if (index >= paramTypes->size() || index < 0) {
            std::cout << "Out of bounds param requested";
            exit(1);
        }
        return (*paramTypes)[index];
    }
    bool equals(TypeGraph *o) override {
        if (this == o) return true;
        if (o->getSubClass() == graphType::TYPE_function &&
            getParamCount() == o->getParamCount()) {
            for (int i = 0; i < getParamCount(); i++) {
                if (!getParamType(i)->equals(o->getParamType(i)))
                    return false;
            }
            return getResultType()->equals(o->getResultType());
        }
        return false;
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
    std::vector<TypeGraph *>* getFields() override { return fields; }
    void addField(TypeGraph *field) override { fields->push_back(field); }
    void setTypeGraph(CustomTypeGraph *owningType) override { customType = owningType; }
    CustomTypeGraph* getCustomType() override { return customType; }
    int getFieldCount() override { return fields->size(); }
    TypeGraph* getFieldType(int index) {
        if (index >= fields->size() || index < 0) {
            std::cout << "Out of bounds param requested";
            exit(1);
        }
        return (*fields)[index];
    }
    bool equals(TypeGraph *o) override {
        if (this == o) return true;
        return o->getSubClass() == graphType::TYPE_record &&
               getCustomType()->equals(o->getCustomType());
    }
    ~ConstructorTypeGraph() { for (auto &field: *fields) if (field->isDeletable()) delete field; }
};
class CustomTypeGraph : public TypeGraph {
    std::vector<ConstructorTypeGraph *> *constructors;
public:
    CustomTypeGraph(std::vector<ConstructorTypeGraph *> *constructors = new std::vector<ConstructorTypeGraph *>())
    : TypeGraph(graphType::TYPE_custom), constructors(constructors) {}
    std::vector<ConstructorTypeGraph *>* getConstructors() { return constructors; }
    int getConstructorCount() override { return constructors->size(); }
    void addConstructor(ConstructorTypeGraph *constructor) override {
        constructors->push_back(constructor);
        constructor->setTypeGraph(this);
    }
    //! possibly too strict. keep an eye out
    bool equals(TypeGraph *o) override {
        // might be of help here
        // o->getSubClass() == graphType::TYPE_custom
        return this == o;
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