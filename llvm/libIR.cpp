#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/IR/Instructions.h>
//#include "lexer.hpp" // to get extern yylineno and not crash from ast include
#include "ast.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <utility>      // std::pair, std::make_pair

// //! ↓↓↓↓↓↓↓↓ Optional ↓↓↓↓↓↓↓↓
// // Get's a function with unit parameters and/or result type and creates an adapter with void
// // This is necessary to create linkable object files with these functions available
// llvm::Function* AST::createFuncAdapterFromUnitToVoid(llvm::Function *unitFunc) {

// }
// // Get's a function with array of char parameter and/or result type and creates an adapter with string
// // This is necessary to create linkable object files with these functions available
// llvm::Function* AST::createFuncAdapterFromCharArrToString(llvm::Function *charArrFunc) {

// }
// //! ↑↑↑↑↑↑↑↑ Optional ↑↑↑↑↑↑↑↑

// Get's a function with void parameter and/or result type and creates an adapter with unit
// This is necessary to link with external, ready libraries
llvm::Function* AST::createFuncAdapterFromVoidToUnit(llvm::Function *voidFunc) {
    llvm::Type *retType = voidFunc->getReturnType();
    std::vector<llvm::Type *> paramTypes = {};
    if (retType->isVoidTy()) { retType = unitType; }
    for (auto &arg: voidFunc->args()) { // copy types to keep them the same
        paramTypes.push_back(arg.getType());
    }
    if (paramTypes.empty()) { paramTypes.push_back(unitType); } // if no args, insert one of type unit
    
    llvm::FunctionType *wrapperFuncType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function *wrapperFunc =  llvm::Function::Create(wrapperFuncType, llvm::Function::InternalLinkage, 
                                  "to.unit." + voidFunc->getName(), TheModule);
    llvm::BasicBlock *wrapperFuncBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", wrapperFunc);
    
    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(wrapperFuncBB);
    std::vector<llvm::Value *> params = {};
    for (auto &arg: wrapperFunc->args()) {
        if (arg.getType() == unitType) 
            continue;
        params.push_back(&arg);
    }
    if (retType == unitType) {
        TmpB.CreateCall(voidFunc, params);
        TmpB.CreateRet(unitVal());
    } else {
        TmpB.CreateRet(TmpB.CreateCall(voidFunc, params, "to.unit.wrapper"));
    }
    TheFPM->run(*wrapperFunc);
    return wrapperFunc;
}
// Get's a function with string parameter and/or result type and creates an adapter with array of char
// This is necessary to link with external, ready libraries
//! Danger: Will change any i8* to array of char, use cautiously
llvm::Function* AST::createFuncAdapterFromStringToCharArr(llvm::Function *stringFunc) {
    llvm::Type *i8ptr = i8->getPointerTo();
    llvm::Type *retType = stringFunc->getReturnType();
    std::vector<llvm::Type *> paramTypes = {};
    // dangerous test
    if (retType == i8ptr) { retType = arrCharType; }
    for (auto &arg: stringFunc->args()) {
        if (arg.getType() == i8ptr) {
            paramTypes.push_back(arrCharType);
        } else {
            paramTypes.push_back(arg.getType());
        }
    }
    llvm::FunctionType *wrapperFuncType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function *wrapperFunc =  llvm::Function::Create(wrapperFuncType, llvm::Function::InternalLinkage, 
                                  "to.chararr." + stringFunc->getName(), TheModule);
    llvm::BasicBlock *wrapperFuncBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", wrapperFunc);

    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(wrapperFuncBB);
    std::vector<llvm::Value *> params = {};
    for (auto &arg: wrapperFunc->args()) {
        if (arg.getType() == arrCharType) {
            llvm::Value *tmpInnerArr = TmpB.CreateGEP(&arg, {c32(0), c32(0)}, "arr.wrap.ptrloc");
            params.push_back(TmpB.CreateLoad(tmpInnerArr));
        } else {
            params.push_back(&arg);
        }
    }
    llvm::Value *retValCandidate = TmpB.CreateCall(stringFunc, params, "to.arrchar.wrapper");
    // std::cout << "Test ORF?\n";
    if (retType == arrCharType) {
        auto *arrayOfCharMalloc =  llvm::CallInst::CreateMalloc(TmpB.GetInsertBlock(), machinePtrType, 
                                                             arrCharType->getPointerElementType(),
                                                             llvm::ConstantExpr::getSizeOf(arrCharType->getPointerElementType()),
                                                             nullptr, 
                                                            //  nullptr,
                                                             TheModule->getFunction("GC_malloc"),
                                                             "to.arrchar.retval");
        llvm::Value *arrayOfCharVal = TmpB.Insert(arrayOfCharMalloc);
        llvm::Value *arrayPtrLoc = TmpB.CreateGEP(arrayOfCharVal, {c32(0), c32(0)}, "to.arrchar.arrayptrloc");
        TmpB.CreateStore(retValCandidate, arrayPtrLoc);
        llvm::Value *dimLoc = TmpB.CreateGEP(arrayOfCharVal, {c32(0), c32(1)}, "to.arrchar.dimloc");
        TmpB.CreateStore(c32(1), dimLoc);
        llvm::Value *sizeLoc = TmpB.CreateGEP(arrayOfCharVal, {c32(0), c32(2)}, "to.arrchar.sizeloc");
        llvm::Value *size = TmpB.CreateCall(TheModule->getFunction("strlen"), {retValCandidate}, "to.arrchar.size");
        TmpB.CreateStore(TmpB.CreateAdd(size, c32(1)), sizeLoc);
        retValCandidate = arrayOfCharVal;
    }
    TmpB.CreateRet(retValCandidate);
    TheFPM->run(*wrapperFunc);
    return wrapperFunc;
}

llvm::Function* adaptReadString(llvm::Function *ReadString, llvm::Module *TheModule,
                                llvm::FunctionType *arrchar_to_unit, llvm::Value *unitVal,
                                llvm::Value *c32_0, llvm::Value *c32_1, llvm::Value *c32_2,
                                llvm::legacy::FunctionPassManager *TheFPM) {
    llvm::Function *readStringAdapted = llvm::Function::Create(arrchar_to_unit, llvm::Function::InternalLinkage,
                                                               "read_string", TheModule);
    llvm::BasicBlock *readStringAdaptedBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", readStringAdapted);
    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(readStringAdaptedBB);
    llvm::Value *readStringArrCharArg = readStringAdapted->getArg(0);
    llvm::Value *readStringStringLoc = TmpB.CreateGEP(readStringArrCharArg, {c32_0, c32_0}, "readString.strloc");
    llvm::Value *readStringSizeLoc = TmpB.CreateGEP(readStringArrCharArg, {c32_0, c32_2}, "readString.sizeloc");
    llvm::Value *readStringSize = TmpB.CreateSub(TmpB.CreateLoad(readStringSizeLoc), c32_1);
    llvm::Value *readStringString = TmpB.CreateLoad(readStringStringLoc);
    TmpB.CreateCall(ReadString, {readStringSize, readStringString});
    TmpB.CreateRet(unitVal);
    TheFPM->run(*readStringAdapted);
    return readStringAdapted;
}

llvm::Function *createIncrLibFunc(llvm::Module *TheModule, llvm::Type *unitType, llvm::Value *c32_1, llvm::Value *unitVal) {
    llvm::FunctionType *intptr_to_unit =
        llvm::FunctionType::get(unitType, {llvm::Type::getInt32PtrTy(TheModule->getContext())}, false);
    llvm::Function *incrFunc = 
        llvm::Function::Create(intptr_to_unit, llvm::Function::InternalLinkage, "incr", TheModule);
    llvm::BasicBlock *incrFuncBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", incrFunc);
    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(incrFuncBB);
    llvm::Value *prevVal = TmpB.CreateLoad(incrFunc->getArg(0), "prevval");
    TmpB.CreateStore(TmpB.CreateAdd(prevVal, c32_1, "newval"), incrFunc->getArg(0));
    TmpB.CreateRet(unitVal);
    // incrFunc->print(llvm::outs());
    return incrFunc;
}

llvm::Function *createDecrLibFunc(llvm::Module *TheModule, llvm::Type *unitType, llvm::Value *c32_1, llvm::Value *unitVal) {
    llvm::FunctionType *intptr_to_unit =
        llvm::FunctionType::get(unitType, {llvm::Type::getInt32PtrTy(TheModule->getContext())}, false);
    llvm::Function *decrFunc = 
        llvm::Function::Create(intptr_to_unit, llvm::Function::InternalLinkage, "decr", TheModule);
    llvm::BasicBlock *decrFuncBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", decrFunc);
    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(decrFuncBB);
    llvm::Value *prevVal = TmpB.CreateLoad(decrFunc->getArg(0), "prevval");
    TmpB.CreateStore(TmpB.CreateSub(prevVal, c32_1, "newval"), decrFunc->getArg(0));
    TmpB.CreateRet(unitVal);
    // decrFunc->print(llvm::outs());
    return decrFunc;
}

llvm::Function *createFloatOfIntLibFunc(llvm::Module *TheModule, llvm::Type *flt) {
    llvm::FunctionType *int_to_float =
        llvm::FunctionType::get(flt, {llvm::Type::getInt32Ty(TheModule->getContext())}, false);
    llvm::Function *floatOfIntFunc =
        llvm::Function::Create(int_to_float, llvm::Function::InternalLinkage, "float_of_int", TheModule);
    llvm::BasicBlock *floatOfIntBB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", floatOfIntFunc);
    llvm::IRBuilder<> TmpB(TheModule->getContext()); TmpB.SetInsertPoint(floatOfIntBB);
    llvm::Value *newFloat = TmpB.CreateCast(llvm::Instruction::SIToFP, floatOfIntFunc->getArg(0), flt, "newfloat");
    TmpB.CreateRet(newFloat);
    // floatOfIntFunc->print(llvm::outs());
    return floatOfIntFunc;
}

std::vector<std::pair<std::string, llvm::Function*>>* AST::genLibGlueLogic() {
    auto pairs = new std::vector<std::pair<std::string, llvm::Function*>>();
    llvm::FunctionType 
        // *int_to_unit = llvm::FunctionType::get(unitType, {i32}, false),
        *int_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i32}, false),
        // *bool_to_unit = llvm::FunctionType::get(unitType, {i1}, false),
        *bool_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i1}, false),
        // *char_to_unit = llvm::FunctionType::get(unitType, {i8}, false),
        *char_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i8}, false),
        // *float_to_unit = llvm::FunctionType::get(unitType, {flt}, false),
        *float_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {flt}, false),
        *arrchar_to_unit = llvm::FunctionType::get(unitType, {arrCharType}, false),
        *string_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i8->getPointerTo()}, false),
        // *unit_to_int = llvm::FunctionType::get(i32, {unitType}, false),
        *void_to_int = llvm::FunctionType::get(i32, {}, false),
        // *unit_to_bool = llvm::FunctionType::get(i1, {unitType}, false),
        *void_to_bool = llvm::FunctionType::get(i1, {}, false),
        // *unit_to_char = llvm::FunctionType::get(i8, {unitType}, false),
        *void_to_char = llvm::FunctionType::get(i8, {}, false),
        // *unit_to_float = llvm::FunctionType::get(flt, {unitType}, false),
        *void_to_float = llvm::FunctionType::get(flt, {}, false),
        // *unit_to_arrchar = llvm::FunctionType::get(arrCharType, {unitType}, false),
        // *void_to_string = llvm::FunctionType::get(i8->getPointerTo(), {}, false),
        *string_to_int = llvm::FunctionType::get(i32, {i8->getPointerTo()}, false),
        *int_string_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i32, i8->getPointerTo()}, false),
        *string_string_to_void = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i8->getPointerTo(), i8->getPointerTo()}, false),
        *string_string_to_int = llvm::FunctionType::get(i32, {i8->getPointerTo(), i8->getPointerTo()}, false);
    llvm::Function 
        *ReadInteger = llvm::Function::Create(void_to_int, llvm::Function::ExternalLinkage, "readInteger", TheModule),
        *ReadBool = llvm::Function::Create(void_to_bool, llvm::Function::ExternalLinkage, "readBoolean", TheModule),
        *ReadChar = llvm::Function::Create(void_to_char, llvm::Function::ExternalLinkage, "readChar", TheModule),
        *ReadFloat = llvm::Function::Create(void_to_float, llvm::Function::ExternalLinkage, "readReal", TheModule),
        *ReadString = llvm::Function::Create(int_string_to_void, llvm::Function::ExternalLinkage, "readString", TheModule),
        *WriteInteger = llvm::Function::Create(int_to_void, llvm::Function::ExternalLinkage, "writeInteger", TheModule),
        *WriteBool = llvm::Function::Create(bool_to_void, llvm::Function::ExternalLinkage, "writeBoolean", TheModule),
        *WriteChar = llvm::Function::Create(char_to_void, llvm::Function::ExternalLinkage, "writeChar", TheModule),
        *WriteFloat = llvm::Function::Create(float_to_void, llvm::Function::ExternalLinkage, "writeReal", TheModule),
        *WriteString = llvm::Function::Create(string_to_void, llvm::Function::ExternalLinkage, "writeString", TheModule),
        *StrLen = llvm::Function::Create(string_to_int, llvm::Function::ExternalLinkage, "strlen", TheModule),
        *StrCpy = llvm::Function::Create(string_string_to_void, llvm::Function::ExternalLinkage, "strcpy", TheModule),
        *StrCmp = llvm::Function::Create(string_string_to_int, llvm::Function::ExternalLinkage, "strcmp", TheModule),
        *StrCat = llvm::Function::Create(string_string_to_void, llvm::Function::ExternalLinkage, "strcat", TheModule);
    std::vector<llvm::Function *> IOlib = {ReadInteger, ReadBool, ReadChar, ReadFloat,
                                           WriteInteger, WriteBool, WriteChar, WriteFloat, WriteString,
                                           StrCpy, StrCat};
    std::vector<llvm::Function *> IOlibAdapted;
    std::transform(IOlib.begin(), IOlib.end(),
        std::back_inserter(IOlibAdapted),
        AST::createFuncAdapterFromVoidToUnit
    );
    IOlibAdapted[8] = createFuncAdapterFromStringToCharArr(IOlibAdapted[8]); // this SHOULD be WriteString
    IOlibAdapted[9] = createFuncAdapterFromStringToCharArr(IOlibAdapted[9]); // this SHOULD be StrCpy
    IOlibAdapted[10] = createFuncAdapterFromStringToCharArr(IOlibAdapted[10]); // this SHOULD be StrCat

    IOlibAdapted.push_back(adaptReadString(ReadString, TheModule, arrchar_to_unit, unitVal(),
                                           c32(0), c32(1), c32(2), TheFPM));
    IOlibAdapted.push_back(createFuncAdapterFromStringToCharArr(StrLen));
    IOlibAdapted.push_back(createFuncAdapterFromStringToCharArr(StrCmp));

    IOlibAdapted[0]->setName("read_int"); IOlibAdapted[1]->setName("read_bool"); 
    IOlibAdapted[2]->setName("read_char"); IOlibAdapted[3]->setName("read_float");
    IOlibAdapted[4]->setName("print_int"); IOlibAdapted[5]->setName("print_bool");
    IOlibAdapted[6]->setName("print_char"); IOlibAdapted[7]->setName("print_float");
    IOlibAdapted[8]->setName("print_string"); IOlibAdapted[11]->setName("read_string");
    // IOlibAdapted[12]->setName("strlen"); // this fails name is same as external
    // IOlibAdapted[13]->setName("strcmp"); // this fails name is same as external
    for (unsigned int i = 0; i < IOlibAdapted.size(); i++) {
        switch (i) {
            case 9: pairs->push_back({"strcpy", IOlibAdapted[i]}); break;
            case 10: pairs->push_back({"strcat", IOlibAdapted[i]}); break;
            case 12: pairs->push_back({"strlen", IOlibAdapted[i]}); break;
            case 13: pairs->push_back({"strcmp", IOlibAdapted[i]}); break;
            default: pairs->push_back({IOlibAdapted[i]->getName(), IOlibAdapted[i]}); 
        }
    }
    llvm::FunctionType 
        *int_to_int = llvm::FunctionType::get(i32, {i32}, false),
        *float_to_float = llvm::FunctionType::get(flt, {flt}, false),
        *float_to_int = llvm::FunctionType::get(i32, {flt}, false),
        *char_to_int = llvm::FunctionType::get(i32, {i8}, false),
        *int_to_char = llvm::FunctionType::get(i8, {i32}, false);
    llvm::Function
        *Abs = llvm::Function::Create(int_to_int, llvm::Function::ExternalLinkage, "abs", TheModule),
        *FAbs = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "fabs", TheModule),
        *Sqrt = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "sqrt", TheModule),
        *Sin = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "sin", TheModule),
        *Cos = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "cos", TheModule),
        *Tan = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "tan", TheModule),
        *Atan = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "atan", TheModule),
        *Exp = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "exp", TheModule),
        *Ln = llvm::Function::Create(float_to_float, llvm::Function::ExternalLinkage, "ln", TheModule),
        *Pi = llvm::Function::Create(void_to_float, llvm::Function::ExternalLinkage, "pi", TheModule),
        *Chr = llvm::Function::Create(int_to_char, llvm::Function::ExternalLinkage, "chr", TheModule),
        *Ord = llvm::Function::Create(char_to_int, llvm::Function::ExternalLinkage, "ord", TheModule),
        *Exit = llvm::Function::Create(int_to_void, llvm::Function::ExternalLinkage, "exit", TheModule),
        *Round = llvm::Function::Create(float_to_int, llvm::Function::ExternalLinkage, "round", TheModule),
        *Trunc = llvm::Function::Create(float_to_int, llvm::Function::ExternalLinkage, "trunc", TheModule);
    std::vector<llvm::Function *> UtilLib = {Abs, FAbs, Sqrt, Sin, Cos, Tan, Atan, Exp, Ln, 
                                             Exit, Round};
    for (auto &func: UtilLib) {
        pairs->push_back({func->getName(), func});
    }
    pairs->push_back({"pi", createFuncAdapterFromVoidToUnit(Pi)});
    pairs->push_back({"int_of_float", Trunc});
    pairs->push_back({"int_of_char", Ord});
    pairs->push_back({"char_of_int", Chr});

    pairs->push_back({"incr", createIncrLibFunc(TheModule, unitType, c32(1), unitVal())});
    pairs->push_back({"decr", createDecrLibFunc(TheModule, unitType, c32(1), unitVal())});
    pairs->push_back({"float_of_int", createFloatOfIntLibFunc(TheModule, flt)});

    llvm::FunctionType *powType = 
        llvm::FunctionType::get(flt, {flt, flt}, false);
    llvm::Function *pow =
        llvm::Function::Create(powType, llvm::Function::ExternalLinkage, "pow.custom", TheModule);
    llvm::BasicBlock *powBB = llvm::BasicBlock::Create(TheContext, "entry", pow);
    llvm::BasicBlock *signApplierBB = llvm::BasicBlock::Create(TheContext, "signapply", pow);
    llvm::BasicBlock *collectorBB = llvm::BasicBlock::Create(TheContext, "collector", pow);
    llvm::IRBuilder<> TmpB(TheContext); TmpB.SetInsertPoint(powBB);

    llvm::Value *isNegative = TmpB.CreateFCmpOLT(pow->getArg(0), f80(0.0), "pow.xisnegative");
    llvm::Value *absX = TmpB.CreateCall(FAbs, {pow->getArg(0)}, "pow.absx");
    llvm::Value *logarithm = TmpB.CreateCall(Ln, {absX}, "pow.lnabsx");
    llvm::Value *mult = TmpB.CreateFMul(pow->getArg(1), logarithm, "pow.ylnx");
    llvm::Value *powRes = TmpB.CreateCall(Exp, {mult}, "pow.res");
    TmpB.CreateCondBr(isNegative, signApplierBB, collectorBB);

    TmpB.SetInsertPoint(signApplierBB);
    llvm::Value *negRes = TmpB.CreateFNeg(powRes);
    TmpB.CreateBr(collectorBB);

    TmpB.SetInsertPoint(collectorBB);
    llvm::PHINode *fullRes = TmpB.CreatePHI(flt, 2, "pow.signrestore");
    fullRes->addIncoming(powRes, powBB);
    fullRes->addIncoming(negRes, signApplierBB);
    TmpB.CreateRet(fullRes);
    
    // for (auto &pair: *pairs) {
    //     std::cout << pair.first << ' ' << pair.second->getName().str() << '\n';
    // }
    return pairs;
}
