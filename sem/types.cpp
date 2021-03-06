#include <iostream>
#include <vector>
#include <string>
#include "types.hpp"
#include "infer.hpp"

/*************************************************************/
/**                    Base TypeGraph                        */
/*************************************************************/

void TypeGraph::wrongCall(std::string functionName) {
    log("Function '" + functionName + "' called for wrong " +
        "subClass, exiting...");
}
TypeGraph::TypeGraph(graphType t): t(t) {}
graphType const & TypeGraph::getSubClass() { return t; }
std::string TypeGraph::stringifyType() {
    return "\033[4m" + stringifyTypeClean() + "\033[0m";
}
std::string TypeGraph::stringifyTypeClean() {
    static const std::string graph_type_string[] = {
     "unknown", "unit", "int", "float", "bool",
     "char", "ref", "array", "function", "custom", "constructor"
    };
    return graph_type_string[(int)(t)];
}
void TypeGraph::log(std::string msg) {
    std::cout << "TypeGraph of type " << stringifyType()
                << " says:" << std::endl << msg << std::endl;
}
bool TypeGraph::isFunction()    { return t == graphType::TYPE_function; }
bool TypeGraph::isArray()       { return t == graphType::TYPE_array;    }
bool TypeGraph::isRef()         { return t == graphType::TYPE_ref;      }
bool TypeGraph::isInt()         { return t == graphType::TYPE_int;      }
bool TypeGraph::isUnit()        { return t == graphType::TYPE_unit;     }
bool TypeGraph::isBool()        { return t == graphType::TYPE_bool;     }
bool TypeGraph::isChar()        { return t == graphType::TYPE_char;     }
bool TypeGraph::isFloat()       { return t == graphType::TYPE_float;    }
bool TypeGraph::isCustom()      { return t == graphType::TYPE_custom;   }
bool TypeGraph::isConstructor() { return t == graphType::TYPE_record;   }
bool TypeGraph::isUnknown()     { return t == graphType::TYPE_unknown;  }
bool TypeGraph::isBasic() {
    return isInt() || isUnit() || isBool() || isChar() || isFloat();
}
bool TypeGraph::isDeletable() { return isFunction() || isArray() || isRef(); }
bool TypeGraph::isUnknownRefOrArray() {
    return (isRef() || isArray()) && (getContainedType()->isUnknown());
}

TypeGraph* TypeGraph::getContainedType() {
    wrongCall("getContainedType()"); exit(1);
}
int TypeGraph::getDimensions() {
    wrongCall("getDimensions()"); exit(1);
}
std::vector<TypeGraph *>* TypeGraph::getParamTypes() {
    wrongCall("getParamTypes()"); exit(1);
}
TypeGraph* TypeGraph::getResultType() {
    wrongCall("getResultType()"); exit(1);
}
int TypeGraph::getParamCount() {
    wrongCall("getParamCount()"); exit(1);
}
void TypeGraph::addParam(TypeGraph *param, bool push_back) {
    wrongCall("addParam()"); exit(1);
}
TypeGraph* TypeGraph::getParamType(unsigned int index) {
    wrongCall("getParamType()"); exit(1);
}
std::vector<TypeGraph *>* TypeGraph::getFields() {
    wrongCall("getFields()"); exit(1);
}
void TypeGraph::addField(TypeGraph *field) { wrongCall("addField"); exit(1); }
void TypeGraph::setTypeGraph(CustomTypeGraph *owningType) { wrongCall("setTypeGraph()"); exit(1); }
CustomTypeGraph* TypeGraph::getCustomType() {
    wrongCall("getCustomType()"); exit(1);
}
int TypeGraph::getFieldCount() {
    wrongCall("getFieldCount()"); exit(1);
}
TypeGraph* TypeGraph::getFieldType(unsigned int index) {
    wrongCall("getFieldType()"); exit(1);
}
int TypeGraph::getConstructorCount() {
    wrongCall("getConstructorCount()"); exit(1);
}
void TypeGraph::addConstructor(ConstructorTypeGraph *constructor) {
    wrongCall("addConstructor()"); exit(1);
}
std::vector<ConstructorTypeGraph *>* TypeGraph::getConstructors() {
    wrongCall("getConstructors()"); exit(1);
}
std::string TypeGraph::getTmpName() {
    wrongCall("getTmpName()"); exit(1);
}
unsigned long TypeGraph::getId() {
    wrongCall("getId()"); exit(1);
}
bool TypeGraph::canBeArray() {
    wrongCall("canBeArray()"); exit(1);
}
bool TypeGraph::canBeFunc() {
    wrongCall("canBeFunc()"); exit(1);
}
bool TypeGraph::onlyIntCharFloat() {
    wrongCall("onlyIntCharFloat()"); exit(1);
}
void TypeGraph::setIntCharFloat() {
    wrongCall("setIntCharFloat()"); exit(1);
}
void TypeGraph::copyConstraintFlags(TypeGraph *o) {
    wrongCall("copyConstraintFlags()"); exit(1);
}
void TypeGraph::changeInner(TypeGraph* replacement, unsigned int index) {
    wrongCall("changeInner()"); exit(1);
}

/*************************************************************/
/**                    Unknown TypeGraph                     */
/*************************************************************/

unsigned long UnknownTypeGraph::curr = 1;
//TODO: not complete
UnknownTypeGraph::UnknownTypeGraph(bool can_be_array, bool can_be_func, bool only_int_char_float):
TypeGraph(graphType::TYPE_unknown), tmp_id(curr++),
can_be_array(can_be_array), can_be_func(can_be_func), only_int_char_float(only_int_char_float) {
    inf.initSubstitution(getTmpName());
}
std::string UnknownTypeGraph::getTmpName() {
    return "@" + std::to_string(tmp_id);
}
unsigned long UnknownTypeGraph::getId() { return tmp_id; }
std::string UnknownTypeGraph::stringifyType() {
    return "\033[4m" + getTmpName() + "\033[0m";
}
std::string UnknownTypeGraph::stringifyTypeClean() {
    return getTmpName();
}
bool UnknownTypeGraph::canBeArray() { return can_be_array; }
bool UnknownTypeGraph::canBeFunc() { return can_be_func; }
bool UnknownTypeGraph::onlyIntCharFloat() { return only_int_char_float; }
void UnknownTypeGraph::setIntCharFloat() { only_int_char_float = true; }
void UnknownTypeGraph::copyConstraintFlags(TypeGraph *o) {
    if (!o->isUnknown()) {
        log("copyConstraintFlags() called with not unknown argument type");
        exit(1);
    }
    can_be_array = can_be_array && o->canBeArray();
    can_be_func = can_be_func && o->canBeFunc();
    only_int_char_float = only_int_char_float || o->onlyIntCharFloat();
}
bool UnknownTypeGraph::equals(TypeGraph *o) {
    return o->isUnknown() && tmp_id == o->getId();
}

/*************************************************************/
/**            Basic TypeGraph and derivatives               */
/*************************************************************/

BasicTypeGraph::BasicTypeGraph(graphType t): TypeGraph(t) {}
bool BasicTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return getSubClass() == o->getSubClass();
}

UnitTypeGraph::UnitTypeGraph()
: BasicTypeGraph(graphType::TYPE_unit) {}
IntTypeGraph::IntTypeGraph()
: BasicTypeGraph(graphType::TYPE_int) {}
CharTypeGraph::CharTypeGraph()
: BasicTypeGraph(graphType::TYPE_char) {}
BoolTypeGraph::BoolTypeGraph()
: BasicTypeGraph(graphType::TYPE_bool) {}
FloatTypeGraph::FloatTypeGraph()
: BasicTypeGraph(graphType::TYPE_float) {}

/*************************************************************/
/**                    Array TypeGraph                       */
/*************************************************************/

ArrayTypeGraph::ArrayTypeGraph(int dimensions, TypeGraph *containedType)
: TypeGraph(graphType::TYPE_array), Type(containedType), dimensions(dimensions) {}
std::string ArrayTypeGraph::stringifyDimensions() {
    if (dimensions == 1) {
        return "array of";
    } else {
        int temp = dimensions - 1;
        std::string retVal = "[*";
        do {
            retVal.append(", *");
        } while (temp -= 1);
        retVal.push_back(']');
        return "array " + retVal + " of";
    }
}
std::string ArrayTypeGraph::stringifyType() {
    return "\033[4m" + 
           stringifyTypeClean() +
           "\033[0m";
}
std::string ArrayTypeGraph::stringifyTypeClean() {
    return stringifyDimensions() + " " +
           getContainedType()->stringifyTypeClean();
}
TypeGraph* ArrayTypeGraph::getContainedType() { return Type; }
int ArrayTypeGraph::getDimensions() { return dimensions; }
bool ArrayTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->isArray() &&
           getContainedType()->equals(o->getContainedType());
}
void ArrayTypeGraph::changeInner(TypeGraph *replacement, unsigned int index) {
    Type = replacement;
}
ArrayTypeGraph::~ArrayTypeGraph() { if (Type->isDeletable()) delete Type; }

/*************************************************************/
/**                    Ref TypeGraph                         */
/*************************************************************/

RefTypeGraph::RefTypeGraph(TypeGraph *refType)
: TypeGraph(graphType::TYPE_ref), Type(refType) {}
std::string RefTypeGraph::stringifyType() {
    return "\033[4m" +
           stringifyTypeClean() + 
           "\033[0m";
}
std::string RefTypeGraph::stringifyTypeClean() {
    return getContainedType()->stringifyTypeClean() + " ref";
}
TypeGraph* RefTypeGraph::getContainedType() { return Type; }
bool RefTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->isRef() &&
           getContainedType()->equals(o->getContainedType());
}
void RefTypeGraph::changeInner(TypeGraph *replacement, unsigned int index) {
    Type = replacement;
}
RefTypeGraph::~RefTypeGraph() { if (Type->isDeletable()) delete Type; }

/*************************************************************/
/**                    Function TypeGraph                    */
/*************************************************************/

FunctionTypeGraph::FunctionTypeGraph(TypeGraph *resultType)
: TypeGraph(graphType::TYPE_function), paramTypes(new std::vector<TypeGraph *>()),
resultType(resultType) {}
std::string FunctionTypeGraph::stringifyParams() {
    if (getParamCount() == 0) {
        return "someFunction";
    }
    std::string retVal = getParamType(0)->stringifyTypeClean();
    std::string temp;
    if (getParamType(0)->isFunction()) {
        retVal.push_back(')');
        retVal.insert(retVal.begin(), '(');
    }
    for (int i = 0; i < getParamCount(); i++) {
        if (i == 0) continue;
        temp = getParamType(i)->stringifyTypeClean();
        if (getParamType(1)->isFunction()) {
            temp.push_back(')');
            temp.insert(temp.begin(), '(');
        }
        retVal.append(" -> " + temp);
    }
    return retVal;
}
std::string FunctionTypeGraph::stringifyType() {
    return "\033[4m" +
            stringifyTypeClean() +
           "\033[0m";
}
std::string FunctionTypeGraph::stringifyTypeClean() {
    return stringifyParams() + " -> " +
           getResultType()->stringifyTypeClean();
}
std::vector<TypeGraph *>* FunctionTypeGraph::getParamTypes() {
    return paramTypes;
}
TypeGraph* FunctionTypeGraph::getResultType() {
    return resultType;
}
int FunctionTypeGraph::getParamCount() {
    return paramTypes->size();
}
void FunctionTypeGraph::addParam(TypeGraph *param, bool push_back) {
    if (push_back)
        paramTypes->push_back(param);
    else
        paramTypes->insert(paramTypes->begin(), param);
}
TypeGraph* FunctionTypeGraph::getParamType(unsigned int index) {
    if (index >= paramTypes->size()) {
        std::cout << "Out of bounds param requested\n";
        exit(1);
    }
    return (*paramTypes)[index];
}

bool FunctionTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    if (o->isFunction() && getParamCount() == o->getParamCount()) {
        for (int i = 0; i < getParamCount(); i++) {
            if (!getParamType(i)->equals(o->getParamType(i)))
                return false;
        }
        return getResultType()->equals(o->getResultType());
    }
    return false;
}
void FunctionTypeGraph::changeInner(TypeGraph *replacement, unsigned int index) {
    if (index > paramTypes->size()) {
        std::cout << "Out of bounds param requested\n";
        exit(1);
    } else if (index == paramTypes->size()) { // no reason this is chosen
        resultType = replacement;
    } else {
        (*paramTypes)[index] = replacement;
    }
}
FunctionTypeGraph::~FunctionTypeGraph() {
    for (auto &paramType: *paramTypes) 
        if (paramType->isDeletable()) delete paramType;
    if (resultType->isDeletable()) delete resultType;;
}

/*************************************************************/
/**                    Constructor TypeGraph                 */
/*************************************************************/

ConstructorTypeGraph::ConstructorTypeGraph(): TypeGraph(graphType::TYPE_record),
fields(new std::vector<TypeGraph *>()), customType(nullptr) {}
std::string ConstructorTypeGraph::stringifyType() {
    return "\033[4m" + stringifyTypeClean() + "\033[0m";
}
std::string ConstructorTypeGraph::stringifyTypeClean() {
    if (getCustomType() == nullptr) {
        return TypeGraph::stringifyTypeClean();
    } else {
        return getCustomType()->stringifyTypeClean();
    }
}
std::vector<TypeGraph *>* ConstructorTypeGraph::getFields() { return fields; }
void ConstructorTypeGraph::addField(TypeGraph *field) { fields->push_back(field); }
void ConstructorTypeGraph::setTypeGraph(CustomTypeGraph *owningType) { customType = owningType; }
CustomTypeGraph* ConstructorTypeGraph::getCustomType() { return customType; }
int ConstructorTypeGraph::getFieldCount() { return fields->size(); }
TypeGraph* ConstructorTypeGraph::getFieldType(unsigned int index) {
    if (index >= fields->size()) {
        std::cout << "Out of bounds constructor field requested";
        exit(1);
    }
    return (*fields)[index];
}
bool ConstructorTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return (o->isConstructor() || o->isCustom()) &&
            getCustomType()->equals(o);
}
ConstructorTypeGraph::~ConstructorTypeGraph() {
    for (auto &field: *fields)
        if (field->isDeletable())
            delete field;
}

/*************************************************************/
/**                     Custom TypeGraph                     */
/*************************************************************/

CustomTypeGraph::CustomTypeGraph(std::string name, std::vector<ConstructorTypeGraph *> *constructors)
: TypeGraph(graphType::TYPE_custom), name(name), constructors(constructors) {}
std::string CustomTypeGraph::stringifyType() {
    return "\033[4m" +
           stringifyTypeClean() +
           "\033[0m";
}
std::string CustomTypeGraph::stringifyTypeClean() {
    return name;
}
std::vector<ConstructorTypeGraph *>* CustomTypeGraph::getConstructors() {
    return constructors;
}
int CustomTypeGraph::getConstructorCount() {
    return constructors->size();
}
void CustomTypeGraph::addConstructor(ConstructorTypeGraph *constructor) {
    constructors->push_back(constructor);
    constructor->setTypeGraph(this);
}
//! possibly too strict. keep an eye out
bool CustomTypeGraph::equals(TypeGraph *o) {
    // might be of help here
    // o->getSubClass() == graphType::TYPE_custom
    if (o->isConstructor())
        return this == o->getCustomType();
    return this == o;
}
CustomTypeGraph::~CustomTypeGraph() {
    for (auto &constructor: *constructors)
        delete constructor;
}
