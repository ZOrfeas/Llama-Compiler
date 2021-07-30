#include <iostream>
#include <vector>
#include <string>
#include "types.hpp"

/*************************************************************/
/**                    Base TypeGraph                        */
/*************************************************************/

void TypeGraph::wrongCall(std::string functionName) {
    log("Function '" + functionName + "' called for wrong " +
        "subClass, exiting...");
    exit(1);
}
TypeGraph::TypeGraph(graphType t): t(t) {}
graphType const & TypeGraph::getSubClass() { return t; }
std::string TypeGraph::stringifyType() {
    return graph_type_string[
        static_cast<int>(getSubClass())
        ];
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

TypeGraph* TypeGraph::getContainedType() {
    wrongCall("getContainedType()"); return nullptr;
}
int TypeGraph::getDimensions() {
    wrongCall("getDimensions()"); return 0;
}
void TypeGraph::setDynamic() {wrongCall("setDynamic()"); }
void TypeGraph::setAllocated() { wrongCall("setAllocated()"); }
void TypeGraph::resetDynamic()  { wrongCall("resetDynamic()"); }
void TypeGraph::resetAllocated() { wrongCall("resetAllocated()"); }
bool TypeGraph::isAllocated() {
    wrongCall("isAllocated()"); return false;
}
bool TypeGraph::isDynamic() {
    wrongCall("isDynamic()"); return false;
}
std::vector<TypeGraph *>* TypeGraph::getParamTypes() {
    wrongCall("getParamTypes()"); return nullptr;
}
TypeGraph* TypeGraph::getResultType() {
    wrongCall("getResultType()"); return nullptr;
}
int TypeGraph::getParamCount() {
    wrongCall("getParamCount()"); return 0;
}
void TypeGraph::addParam(TypeGraph *param, bool push_back) {
    wrongCall("addParam()");
}
TypeGraph* TypeGraph::getParamType(unsigned int index) {
    wrongCall("getParamType()"); return nullptr;
}
std::vector<TypeGraph *>* TypeGraph::getFields() {
    wrongCall("getFields()"); return nullptr;
}
void TypeGraph::addField(TypeGraph *field) { wrongCall("addField"); }
void TypeGraph::setTypeGraph(CustomTypeGraph *owningType) { wrongCall("setTypeGraph()"); }
CustomTypeGraph* TypeGraph::getCustomType() {
    wrongCall("getCustomType()"); return nullptr;
}
int TypeGraph::getFieldCount() {
    wrongCall("getFieldCount()"); return 0;
}
TypeGraph* TypeGraph::getFieldType(unsigned int index) {
    wrongCall("getFieldType()"); return nullptr;
}
int TypeGraph::getConstructorCount() {
    wrongCall("getConstructorCount()"); return 0;
}
void TypeGraph::addConstructor(ConstructorTypeGraph *constructor) {
    wrongCall("addConstructor()");
}
std::vector<ConstructorTypeGraph *>* TypeGraph::getConstructors() {
    wrongCall("getConstructors()"); return nullptr;
}

/*************************************************************/
/**                    Unknown TypeGraph                     */
/*************************************************************/

UnknownTypeGraph::UnknownTypeGraph():
TypeGraph(graphType::TYPE_unknown) {}
//TODO: not complete
bool UnknownTypeGraph::equals(TypeGraph *o) { return false; }

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
TypeGraph* ArrayTypeGraph::getContainedType() { return Type; }
int ArrayTypeGraph::getDimensions() { return dimensions; }
bool ArrayTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->getSubClass() == graphType::TYPE_array &&
            getContainedType()->equals(o->getContainedType());
}
ArrayTypeGraph::~ArrayTypeGraph() { if (Type->isDeletable()) delete Type; }

/*************************************************************/
/**                    Ref TypeGraph                         */
/*************************************************************/

RefTypeGraph::RefTypeGraph(TypeGraph *refType, bool allocated, bool dynamic)
: TypeGraph(graphType::TYPE_ref), Type(refType), allocated(allocated), dynamic(dynamic) {}
TypeGraph* RefTypeGraph::getContainedType() { return Type; }
void RefTypeGraph::setAllocated() { allocated = true; }
void RefTypeGraph::setDynamic() { dynamic = true; }
void RefTypeGraph::resetAllocated() { allocated = false; }
void RefTypeGraph::resetDynamic() { dynamic = false; }
bool RefTypeGraph::isAllocated() { return allocated; }
bool RefTypeGraph::isDynamic() { return dynamic; }
bool RefTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->getSubClass() == graphType::TYPE_ref &&
            getContainedType()->equals(o->getContainedType());
}
RefTypeGraph::~RefTypeGraph() { if (Type->isDeletable()) delete Type; }

/*************************************************************/
/**                    Function TypeGraph                    */
/*************************************************************/

FunctionTypeGraph::FunctionTypeGraph(TypeGraph *resultType)
: TypeGraph(graphType::TYPE_function), paramTypes(new std::vector<TypeGraph *>()),
resultType(resultType) {}
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
        std::cout << "Out of bounds param requested";
        exit(1);
    }
    return (*paramTypes)[index];
}

bool FunctionTypeGraph::equals(TypeGraph *o) {
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
std::vector<TypeGraph *>* ConstructorTypeGraph::getFields() { return fields; }
void ConstructorTypeGraph::addField(TypeGraph *field) { fields->push_back(field); }
void ConstructorTypeGraph::setTypeGraph(CustomTypeGraph *owningType) { customType = owningType; }
CustomTypeGraph* ConstructorTypeGraph::getCustomType() { return customType; }
int ConstructorTypeGraph::getFieldCount() { return fields->size(); }
TypeGraph* ConstructorTypeGraph::getFieldType(unsigned int index) {
    if (index >= fields->size()) {
        std::cout << "Out of bounds param requested";
        exit(1);
    }
    return (*fields)[index];
}
bool ConstructorTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->getSubClass() == graphType::TYPE_record &&
            getCustomType()->equals(o->getCustomType());
}
ConstructorTypeGraph::~ConstructorTypeGraph() {
    for (auto &field: *fields)
        if (field->isDeletable())
            delete field;
}

/*************************************************************/
/**                    Constructor TypeGraph                 */
/*************************************************************/

CustomTypeGraph::CustomTypeGraph(std::vector<ConstructorTypeGraph *> *constructors)
: TypeGraph(graphType::TYPE_custom), constructors(constructors) {}
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
    return this == o;
}
CustomTypeGraph::~CustomTypeGraph() {
    for (auto &constructor: *constructors)
        delete constructor;
}
