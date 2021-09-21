#include <iostream>
#include <vector>
#include <string>
#include "types.hpp"
#include "infer.hpp"
#include "ast.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>

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
int TypeGraph::getBound() {
    wrongCall("getBound()"); exit(1);
}
int *TypeGraph::getBoundPtr() {
    wrongCall("getBoundPtr()"); exit(1);
}
void TypeGraph::changeBoundVal(int newBound) {
    wrongCall("changeBoundVal()"); exit(1);
}
void TypeGraph::changeBoundPtr(int *newBoundPtr) {
    wrongCall("changeBoundPtr()"); exit(1);
}
void TypeGraph::setDimensions(int fixedDimensions) {
    wrongCall("setDimensions()"); exit(1);
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

ArrayTypeGraph::ArrayTypeGraph(int dimensions, TypeGraph *containedType, int lowBound)
: TypeGraph(graphType::TYPE_array), Type(containedType),
dimensions(dimensions),
lowBound(new int(lowBound)) {}
std::string ArrayTypeGraph::stringifyDimensions() {
    if (dimensions == 1) {
        return "array of";
    } else if (dimensions == -1) {
        return std::string("array (at least ") + std::to_string(*lowBound) + ") of";
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
    return std::string("(") + stringifyDimensions() + " " +
           getContainedType()->stringifyTypeClean() + std::string(")");
}
TypeGraph* ArrayTypeGraph::getContainedType() { return Type; }
int ArrayTypeGraph::getDimensions() {
    return *lowBound != -1 && dimensions != -1 ? *lowBound : dimensions;
}
bool ArrayTypeGraph::equals(TypeGraph *o) {
    if (this == o) return true;
    return o->isArray() &&
           getContainedType()->equals(o->getContainedType());
}
void ArrayTypeGraph::changeInner(TypeGraph *replacement, unsigned int index) {
    Type = replacement;
}
int ArrayTypeGraph::getBound() { return *lowBound; }
int *ArrayTypeGraph::getBoundPtr() { return lowBound; }
void ArrayTypeGraph::changeBoundVal(int newBound) {
    dimensions = -1; // this 'disables' dimensions
    *lowBound = newBound;
}
void ArrayTypeGraph::changeBoundPtr(int *newBoundPtr) {
    dimensions = -1; // this 'disables' dimensions
    lowBound = newBoundPtr;
}
void ArrayTypeGraph::setDimensions(int fixedDimensions) {
    dimensions = fixedDimensions; // sets personal int
    *lowBound = fixedDimensions; // sets the positions int
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

ConstructorTypeGraph::ConstructorTypeGraph(std::string name):TypeGraph(graphType::TYPE_record),
customType(nullptr), name(name),
fields(new std::vector<TypeGraph *>()) {}
std::string ConstructorTypeGraph::stringifyType() {
    return "\033[4m" + stringifyTypeClean() + "\033[0m";
}
std::string ConstructorTypeGraph::stringifyTypeClean() {
    return name;
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
int ConstructorTypeGraph::getIndex()
{
    // If it hasn't yet been found find it and save it
    if(index == -1)
    {
        index = customType->getConstructorIndex(name);
    }

    return index;
}
std::string ConstructorTypeGraph::getName() 
{
    return name;
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
: TypeGraph(graphType::TYPE_custom), name(name), constructors(constructors), structEqFunc(nullptr) {}
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
int CustomTypeGraph::getConstructorIndex(ConstructorTypeGraph *c)
{
    // NOTE: This doesn't work correctly
    for(long unsigned int i = 0; i < constructors->size(); i++)
    {
        if((*constructors)[i]->equals(c)) 
        {
            return i;
        }
    }
    exit(1);
}
int CustomTypeGraph::getConstructorIndex(std::string Id) 
{
    std::string s;
    for(long unsigned int i = 0; i < constructors->size(); i++)
    {
        s = (*constructors)[i]->getName();
        if(Id == s) 
        {
            return i;
        }
    }
    exit(1);
}
CustomTypeGraph::~CustomTypeGraph() {
    for (auto &constructor: *constructors)
        delete constructor;
}

/*************************************************************/
/**                     LLVM Functions                       */
/*************************************************************/

llvm::Type* TypeGraph::getLLVMType(llvm::Module *TheModule){
    return nullptr;
}
llvm::Type* UnknownTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    TypeGraph *correctTypeGraph = inf.deepSubstitute(this);
    return correctTypeGraph->getLLVMType(TheModule);
}
llvm::Type* UnitTypeGraph::getLLVMType(llvm::Module *TheModule) 
{
    if (llvm::Type* unitType = TheModule->getTypeByName("unit")) {
        return unitType;
    } else {
        llvm::StructType* newUnitType = llvm::StructType::create(TheModule->getContext());
        newUnitType->setName("unit");
        newUnitType->setBody({llvm::Type::getInt1Ty(TheModule->getContext())});
        return newUnitType;
    }
}
llvm::IntegerType* IntTypeGraph::getLLVMType(llvm::Module *TheModule)
{ 
    return llvm::Type::getInt32Ty(TheModule->getContext());
}
llvm::IntegerType* CharTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    return llvm::Type::getInt8Ty(TheModule->getContext());
}
llvm::IntegerType* BoolTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    return llvm::Type::getInt1Ty(TheModule->getContext());
}
llvm::Type* FloatTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    return llvm::Type::getX86_FP80Ty(TheModule->getContext());
}
llvm::PointerType* ArrayTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    // Get element type
    TypeGraph *elementTypeGraph = inf.deepSubstitute(this)->getContainedType()->getContainedType();
    llvm::Type *elementLLVMType = elementTypeGraph->getLLVMType(TheModule);

    // Prepare name of type
    std::string arrayTypeName = std::string("Array") + 
                                '.' + std::to_string(dimensions) + 
                                '.' + elementTypeGraph->stringifyTypeClean(); 

    // Check if it exists
    llvm::StructType *LLVMArrayType;
    if((LLVMArrayType = TheModule->getTypeByName(arrayTypeName)))
    {
        return LLVMArrayType->getPointerTo();
    }

    // Vector for StructType members
    std::vector<llvm::Type *> members;

    // Create PointerType to which the actual array will be malloc'd
    llvm::PointerType *arrayPointer = elementLLVMType->getPointerTo();
    members.push_back(arrayPointer);
    
    // Create all the integer types for dimensions and dimension sizes
    llvm::IntegerType *LLVMInteger = llvm::Type::getInt32Ty(TheModule->getContext());
    members.insert(members.end(), dimensions + 1, LLVMInteger);
    
    // Create StructType that will be used to represent arrays
    LLVMArrayType = llvm::StructType::create(TheModule->getContext(), arrayTypeName);
    LLVMArrayType->setBody(members);

    return LLVMArrayType->getPointerTo();
}
llvm::PointerType* RefTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    llvm::Type *containedLLVMType = this->Type->getLLVMType(TheModule);
    return containedLLVMType->getPointerTo();
}
llvm::PointerType* FunctionTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    llvm::Type *LLVMResultType = resultType->getLLVMType(TheModule);

    std::vector<llvm::Type *> LLVMParamTypes = {};
    for(auto p: *paramTypes)
    {
        LLVMParamTypes.push_back(p->getLLVMType(TheModule));
    }
    llvm::FunctionType *tempType = llvm::FunctionType::get(LLVMResultType, LLVMParamTypes, false);
    // This returns a pointer of the function type
    return tempType->getPointerTo();
}
llvm::StructType* ConstructorTypeGraph::getLLVMType(llvm::Module *TheModule)
{
    std::vector<llvm::Type *> LLVMTypeList = {};

    // Add the rest of the fields of the constructor
    for(auto f: *fields)
    {
        LLVMTypeList.push_back(f->getLLVMType(TheModule));
    }

    return llvm::StructType::get(TheModule->getContext(), LLVMTypeList);
}
/*
    Custom Types will be implemented as a struct
    containing a single field that corresponds to the
    largest constructor type. Essentially implements a union.

    This either creates the type or returns it if it has already 
    been created.
*/
llvm::PointerType* CustomTypeGraph::getLLVMType(llvm::Module *TheModule)
{   
    llvm::StructType *LLVMCustomType;
    if ((LLVMCustomType = TheModule->getTypeByName(name))) {
        return LLVMCustomType->getPointerTo();
    }

    LLVMCustomType = llvm::StructType::create(TheModule->getContext(), name);
    
    const llvm::DataLayout &TheDataLayout = TheModule->getDataLayout();
    llvm::StructType *LLVMLargestStructType, *LLVMTempType;
    bool first = true;
    int prevSize, currSize;
    for(auto c: *constructors)
    {
        LLVMTempType = c->getLLVMType(TheModule);

        // The first time just initialise the the StructType
        if(first) 
        {
            LLVMLargestStructType = LLVMTempType;
            first = false;
            continue;
        }

        prevSize = TheDataLayout.getTypeAllocSize(LLVMLargestStructType);
        currSize = TheDataLayout.getTypeAllocSize(LLVMTempType);
        if(prevSize < currSize) LLVMLargestStructType = LLVMTempType;
    }

    // Create a StructType with one ConstantInt and the LargestStructType
    llvm::IntegerType *LLVMStructEnum = llvm::Type::getInt32Ty(TheModule->getContext());
    std::vector<llvm::Type *> LLVMStructTypes = { LLVMStructEnum, LLVMLargestStructType };
    LLVMCustomType->setBody(LLVMStructTypes);
    
    return LLVMCustomType->getPointerTo();
}

// defined here instead of genIR.cpp for linking order reasons
llvm::Value *AST::equalityHelper(llvm::Value *lhsVal,
                                 llvm::Value *rhsVal,
                                 TypeGraph *type,
                                 bool structural,
                                 llvm::IRBuilder<> TmpB)
{
    if (type->isUnit()) {
        return c1(true);
    }
    if (type->isCustom() && structural)
    {   
        if (CustomTypeGraph *tmpCstType = dynamic_cast<CustomTypeGraph*>(type)) {
            llvm::Function *cstTypeEqFunc = tmpCstType->getStructEqFunc(TheModule, TheFPM);
            return TmpB.CreateCall(cstTypeEqFunc, {lhsVal, rhsVal}, "strcteq.equals");
        }
        else { // internal error
            std::cerr << "Internal error: equalityHelper impossible else entered\n";
            exit(1);
        }
    }
    if ((type->isCustom() && !structural) || type->isRef()) {
        llvm::Value 
            *lhsPointerInt = TmpB.CreatePtrToInt(lhsVal, machinePtrType, "ptr.cmplhstmp"),
            *rhsPointerInt = TmpB.CreatePtrToInt(rhsVal, machinePtrType, "ptr.cmprhstmp");
        return TmpB.CreateICmpEQ(lhsPointerInt, rhsPointerInt, "ptr.cmpnateqtmp");         
    }
    if (type->isInt() || type->isBool() || type->isChar()) {
        return TmpB.CreateICmpEQ(lhsVal, rhsVal, "int.cmpeqtmp");
    }
    if (type->isFloat()) {
        return TmpB.CreateFCmpOEQ(lhsVal, rhsVal, "float.cmpeqtmp");
    }
    std::cerr << "Structural equality attempted of custom types containing array or function field\n";
    exit(1);
}


llvm::Function *CustomTypeGraph::getStructEqFunc(llvm::Module *TheModule, 
                                                 llvm::legacy::FunctionPassManager *TheFPM) {

    // if it has been already declared and saved, then just return it
    if (structEqFunc)
        return structEqFunc;

    // else declare and define it
    auto &TheContext = TheModule->getContext();
    auto c32 = [&](int n) { 
        return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, false));
    };
    auto c1 = [&](bool b) {
        return llvm::ConstantInt::get(TheContext, llvm::APInt(1, b, false));
    };
    auto *structLLVMType = getLLVMType(TheModule);
    auto *eqFuncType = llvm::FunctionType::get(
        llvm::Type::getInt1Ty(TheContext),
        {structLLVMType, structLLVMType},
        false
    );
    structEqFunc = llvm::Function::Create(
        eqFuncType,
        llvm::Function::InternalLinkage,
        name + ".strcteq",
        TheModule
    );
    llvm::Value *lhsVal = structEqFunc->getArg(0),
                *rhsVal = structEqFunc->getArg(1);

    auto *entryBB = llvm::BasicBlock::Create(TheContext, "entry", structEqFunc),
         *exitBB = llvm::BasicBlock::Create(TheContext, "exit", structEqFunc),
         *switchBB = llvm::BasicBlock::Create(TheContext, "switch.init", structEqFunc),
         *errorBB = llvm::BasicBlock::Create(TheContext, "error", structEqFunc);
    llvm::IRBuilder<> TmpB(TheContext);
    TmpB.SetInsertPoint(exitBB);
    std::size_t incomingCount = 0;
    for (auto &constr: *constructors) {
        std::size_t fieldCount = constr->getFieldCount();
        // if it has no fields, still add one incoming
        incomingCount += fieldCount ? fieldCount : 1;
    }
    auto *resPhi = TmpB.CreatePHI(
        llvm::Type::getInt1Ty(TheContext),
        1 + incomingCount, //TODO(ORF): Make sure this is correct
        name + ".strcteq.res"
    );

    // Holders for fields being compared every moment
    llvm::Value *lhsFieldLoc, *lhsField, *rhsFieldLoc, *rhsField, *compRes;
    TmpB.SetInsertPoint(entryBB);
    lhsFieldLoc = TmpB.CreateGEP(lhsVal, {c32(0), c32(0)}, "strcteq.lhstypeloc");
    lhsField = TmpB.CreateLoad(lhsFieldLoc);
    rhsFieldLoc = TmpB.CreateGEP(rhsVal, {c32(0), c32(0)}, "strcteq.rhstypeloc");
    rhsField = TmpB.CreateLoad(rhsFieldLoc);
    compRes = TmpB.CreateICmpEQ(
        lhsField,
        rhsField,
        "strcteq.sametypetmp"
    );
    // comparison fails if not of the same constructor type
    TmpB.CreateCondBr(compRes, switchBB, exitBB);
    resPhi->addIncoming(compRes, entryBB);

    // switch logic init
    TmpB.SetInsertPoint(switchBB);
    llvm::Value *lhsInnerStructLoc = TmpB.CreateGEP(
                    lhsVal, {c32(0), c32(1)}, "strcteq.lhsconstrloc"),
                *rhsInnerStructLoc = TmpB.CreateGEP(
                    rhsVal, {c32(0), c32(1)}, "strcteq.rhsconstrloc");
    llvm::Value *constrType = lhsField; // save the type of constr it is
    std::vector<llvm::BasicBlock *> switchTypeBBs;
    auto *typeSwitch = 
        TmpB.CreateSwitch(constrType, errorBB, constructors->size());
    llvm::BasicBlock *currentBB;
    for (std::size_t i = 0; i < constructors->size(); i++) {
        // one switch case for each constructor type
        currentBB = 
            llvm::BasicBlock::Create(
                TheContext,
                std::string("case.") + (*constructors)[i]->getName(),
                structEqFunc
            );
        switchTypeBBs.push_back(currentBB);
        typeSwitch->addCase(c32(i), currentBB);
    }
    
    // default/error BB code
    TmpB.SetInsertPoint(errorBB);
    TmpB.CreateCall(TheModule->getFunction("writeString"),
        {TmpB.CreateGlobalStringPtr("Internal error: Invalid constructor enum\n")});
    TmpB.CreateCall(TheModule->getFunction("exit"), {c32(1)});
    TmpB.CreateBr(errorBB); // necessary to avoid llvm error

    // logic inside each switch case
    // for every constructor type
    for (std::size_t i = 0; i < constructors->size(); i++) {
        llvm::Value *lhsCastedVal, *rhsCastedVal;
        ConstructorTypeGraph *currConstrGraph = (*constructors)[i];
        llvm::StructType *currConstrType = currConstrGraph->getLLVMType(TheModule);
        currentBB = switchTypeBBs[i];
        TmpB.SetInsertPoint(currentBB);
        
        // if it has no fields
        if (currConstrGraph->getFieldCount() == 0) {
            TmpB.CreateBr(exitBB);
            resPhi->addIncoming(c1(true), currentBB);
            continue;
        }

        lhsCastedVal = TmpB.CreatePointerCast(
            lhsInnerStructLoc, currConstrType->getPointerTo(), "strcteq.lhscast");
        rhsCastedVal = TmpB.CreatePointerCast(
            rhsInnerStructLoc, currConstrType->getPointerTo(), "strcteq.rhscast");
        // for every field of constructor
        for (int j = 0; j < currConstrGraph->getFieldCount(); j++) {
            lhsFieldLoc = TmpB.CreateGEP(
                lhsCastedVal, {c32(0), c32(j)}, "strcteq.lhsfieldloc");
            lhsField = TmpB.CreateLoad(lhsFieldLoc);
            rhsFieldLoc = TmpB.CreateGEP(
                rhsCastedVal, {c32(0), c32(j)}, "strcteq.rhsfieldloc");
            rhsField = TmpB.CreateLoad(rhsFieldLoc);
            compRes = AST::equalityHelper(
                lhsField, rhsField, currConstrGraph->getFieldType(j), true, TmpB);

            // if it's not the last field            
            if (j != currConstrGraph->getFieldCount() - 1) {
                llvm::BasicBlock *nextFieldBB = 
                    llvm::BasicBlock::Create(
                        TheContext, 
                        std::string("case.") + currConstrGraph->getName() + ".nextfield", 
                        structEqFunc);
                TmpB.CreateCondBr(compRes, nextFieldBB, exitBB);
                resPhi->addIncoming(compRes, TmpB.GetInsertBlock());
                TmpB.SetInsertPoint(nextFieldBB);
            } else { // if it's the final field
                TmpB.CreateBr(exitBB);
                resPhi->addIncoming(compRes, TmpB.GetInsertBlock());
            }
        }
    }
    TmpB.SetInsertPoint(exitBB);
    TmpB.CreateRet(resPhi);

    TheFPM->run(*structEqFunc);

    return structEqFunc;
}