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
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
//#include "lexer.hpp" // to get extern yylineno and not crash from ast include
#include "ast.hpp"
#include "infer.hpp"
#include "parser.hpp"
#include <map>
#include <vector>
#include <string>
#include <utility> // std::pair, std::make_pair

/*********************************/
/**        Symbol-Tables         */
/*********************************/

/** @param T is the Type the table will will hold pointers of */
template <class T>
class LLTable
{
    std::vector<std::map<std::string, T> *> *table;
    bool nameInScope(std::string name, std::map<std::string, T> *scope)
    {
        return (scope->find(name) != scope->end());
    }

public:
    LLTable() : table(new std::vector<std::map<std::string, T> *>())
    {
        openScope(); // open first (global) scope
    }
    /** Can be called with the implicit pair constructor
     * e.g.: insert({"a_name", a_pointer})
    */
    void insert(std::pair<std::string, T> entry)
    {
        (*table->back())[entry.first] = entry.second;
    }
    T operator[](std::string name)
    {
        for (auto it = table->rbegin(); it != table->rend(); it++)
        {
            if (nameInScope(name, *it))
                return (**it)[name];
        }
        return nullptr;
    }
    void openScope()
    {
        table->push_back(new std::map<std::string, T>());
    }
    void closeScope()
    {
        if (table->size() != 0)
        {
            std::map<std::string, T> *poppedScope = table->back();
            table->pop_back();
            delete poppedScope;
        }
    }
    int getCurrScope()
    {
        return (table->size() - 1);
    }
    int getScopeOf(std::string name)
    {
        int scope = getCurrScope();
        for (auto it = table->rbegin(); it != table->rend(); it++)
        {
            scope--;
            if (nameInScope(name, *it))
                break;
        }
        return scope;
    }
    ~LLTable() {}
};
LLTable<llvm::Value *> LLValues;

/*
// Keeps track of the scope of the function inside which we are writing
std::vector<int> functionScopeStack = {0};
llvm::Value *accessSymbolOrMakeGlobal(std::string name)
{
    int currFuncScope = functionScopeStack.back();
    int symbolScope = LLValues.getScopeOf(name);

    // If the symbol was defined inside the function body all good
    if (currFuncScope <= symbolScope)
    {
        return LLValues[name];
    }
}
*/

void openScopeOfAll()
{
    LLValues.openScope();
}
void closeScopeOfAll()
{
    LLValues.closeScope();
}


/*********************************/
/**          Utilities           */
/*********************************/

llvm::ConstantInt *AST::c1(bool b)
{
    return llvm::ConstantInt::get(TheContext, llvm::APInt(1, b, false));
}
llvm::ConstantInt *AST::c8(char c)
{
    return llvm::ConstantInt::get(TheContext, llvm::APInt(8, c, true));
}
llvm::ConstantInt *AST::c32(int n)
{
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, true));
}
llvm::ConstantInt *AST::c64(long int n)
{
    return llvm::ConstantInt::get(TheContext, llvm::APInt(64, n, true));
}
llvm::Constant *AST::f80(long double d)
{
    return llvm::ConstantFP::get(flt, d);
}
llvm::Constant *AST::unitVal()
{
    return llvm::Constant::getNullValue(unitType);
}

// Creates alloca of specified name and type and inserts it at the beginning of the block
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                                const std::string &VarName,
                                                llvm::Type *LLVMType)
{
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(LLVMType, nullptr, VarName.c_str());
}

std::map<std::string, llvm::Value *> declaredGlobalStrings;
bool stringDeclared(std::string s)
{
    return declaredGlobalStrings.find(s) != declaredGlobalStrings.end();
}
llvm::Value *getGlobalString(std::string s, llvm::IRBuilder<> Builder)
{
    if (!stringDeclared(s))
        declaredGlobalStrings[s] = Builder.CreateGlobalStringPtr(s);
    return declaredGlobalStrings[s];

}

/*********************************/
/**       Initializations        */
/*********************************/

llvm::LLVMContext AST::TheContext;
llvm::IRBuilder<> AST::Builder(AST::TheContext);
llvm::Module *AST::TheModule;
llvm::legacy::FunctionPassManager *AST::TheFPM;

llvm::TargetMachine *AST::TargetMachine;

llvm::Type *AST::i1;
llvm::Type *AST::i8;
llvm::Type *AST::i32;
llvm::Type *AST::flt;
llvm::Type *AST::unitType;
llvm::Type *AST::machinePtrType;
llvm::Type *AST::arrCharType;

llvm::Function *AST::TheMalloc;
llvm::Function *AST::TheUncollectableMalloc;

void AST::start_compilation(const char *programName, bool optimize)
{
    TheModule = new llvm::Module(programName, TheContext);
    TheFPM = new llvm::legacy::FunctionPassManager(TheModule);
    if (optimize)
    {
        TheFPM->add(llvm::createPromoteMemoryToRegisterPass());
        TheFPM->add(llvm::createInstructionCombiningPass());
        TheFPM->add(llvm::createReassociatePass());
        TheFPM->add(llvm::createGVNPass());
        TheFPM->add(llvm::createCFGSimplificationPass());
    }
    TheFPM->doInitialization();
    // Emit object code initializations
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target)
    {
        llvm::errs() << Error;
        exit(1);
    }
    auto CPU = "generic";
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    TheModule->setDataLayout(TargetMachine->createDataLayout());
    TheModule->setTargetTriple(TargetTriple);
    // Basic types initializations start
    i1 = type_bool->getLLVMType(TheModule);
    i8 = type_char->getLLVMType(TheModule);
    i32 = type_int->getLLVMType(TheModule);
    flt = type_float->getLLVMType(TheModule);
    unitType = type_unit->getLLVMType(TheModule);
    machinePtrType = llvm::Type::getIntNTy(TheContext, TheModule->getDataLayout().getMaxPointerSizeInBits());
    arrCharType = (new ArrayTypeGraph(1, new RefTypeGraph(type_char)))->getLLVMType(TheModule);
    // Initialize runtime lib functions
    std::vector<std::pair<std::string, llvm::Function *>> *libFunctions = genLibGlueLogic();
    for (auto &libFunc : *libFunctions)
    {
        LLValues.insert(libFunc);
    }
    // Initialize garbage collection functions
#ifdef LIBGC
    llvm::FunctionType *gcMallocType = llvm::FunctionType::get(i8->getPointerTo(), {machinePtrType}, false);
    TheMalloc = llvm::Function::Create(gcMallocType, llvm::Function::ExternalLinkage,
                           "GC_malloc_atomic", TheModule);
    TheUncollectableMalloc = llvm::Function::Create(gcMallocType, llvm::Function::ExternalLinkage,
                           "GC_malloc_atomic_uncollectable", TheModule);
    llvm::FunctionType *gcFreeType = llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i8->getPointerTo()}, false);
    llvm::Function::Create(gcFreeType, llvm::Function::ExternalLinkage,
                           "GC_free", TheModule);
    // Initialize main function (entry point)
#else
    TheMalloc = TheUncollectableMalloc = nullptr;
#endif // LIBGC
    llvm::FunctionType *main_type = llvm::FunctionType::get(i32, {}, false);
    llvm::Function *main =
        llvm::Function::Create(main_type, llvm::Function::ExternalLinkage,
                               "main", TheModule);
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", main);
    Builder.SetInsertPoint(BB);
    // compile the program code
    compile();
    Builder.CreateRet(c32(0));

    bool bad = llvm::verifyModule(*TheModule, &llvm::errs());
    if (bad)
    { // internal error
        std::cerr << "The IR is bad!" << std::endl;
        TheModule->print(llvm::errs(), nullptr);
        std::exit(1);
    }
    TheFPM->run(*main);
}
void AST::printLLVMIR()
{
    TheModule->print(llvm::outs(), nullptr);
}
void AST::emitObjectCode(const char *filename)
{
    std::error_code EC;
    llvm::raw_fd_ostream dst(filename, EC, llvm::sys::fs::OF_None);

    if (EC)
    {
        llvm::errs() << "Could not open file: " << EC.message();
        exit(1);
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dst, nullptr, FileType))
    {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        exit(1);
    }

    pass.run(*TheModule);
    dst.flush();
}
void AST::emitAssemblyCode()
{
    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_AssemblyFile;

    if (TargetMachine->addPassesToEmitFile(pass, (llvm::raw_pwrite_stream &)llvm::outs(),
                                           nullptr, FileType))
    {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }
    pass.run(*TheModule);
    llvm::outs().flush();
}

llvm::Value *AST::getGlobalLiveValue() {
    return globalLiveValue;
}

llvm::Value* AST::updateGlobalValue(llvm::Value *newVal) {
    if (listOfFunctionsThatNeedSymbol.empty())
        return nullptr;
    if (!globalLiveValue) {
        auto initializer = llvm::ConstantAggregateZero::get(newVal->getType());
        globalLiveValue = new llvm::GlobalVariable(
            *TheModule,
            newVal->getType(),
            false,
            llvm::GlobalValue::InternalLinkage,
            initializer
        );
    }
    auto prevGlobal = Builder.CreateLoad(globalLiveValue, "reminder");
    Builder.CreateStore(newVal, globalLiveValue);
    return prevGlobal;
}

/*********************************/
/**        Definitions           */
/*********************************/

llvm::Value *AST::compile()
{
    return nullptr;
}
llvm::Value *Constr::compile()
{
    return nullptr;
}
llvm::Value *Tdef::compile()
{
    return nullptr;
}
llvm::Value *Constant::compile()
{
    // std::cerr << "Constant " << id << " is needed by functions: ";
    // for(auto f: listOfFunctionsThatNeedSymbol)
    // {
    //     std::cerr   << f->getId() 
    //                 << "("
    //                 << f->getTypeGraph()->stringifyTypeClean() 
    //                 << ") ";
    // }
    // std::cerr << std::endl;

    llvm::Value *exprVal = expr->compile();
    exprVal->setName(id);
    LLValues.insert({id, exprVal});
    updateGlobalValue(exprVal);
    return nullptr;
}

llvm::StructType *Function::getEnvStructType()
{
    if (envStructType)
        return envStructType;
    std::string envTypeName = getId() + ".env";
    ConstructorTypeGraph *utilGraph = new ConstructorTypeGraph(envTypeName);
    for (const auto &ext: external) {
        utilGraph->addField(ext.second->getTypeGraph());
    }
    envStructType = utilGraph->getLLVMType(TheModule);
    envStructType->setName(envTypeName);
    return envStructType;
}

void DefStmt::generateLLVMPrototype() {
    std::cerr << "generateLLVMPrototype called for DefStmt\n";
    exit(1);
}
void Function::generateLLVMPrototype()
{
    // std::cerr << "Function " << id << " needs symbols: ";
    // for(auto e: external)
    // {
    //     std::cerr   << e.first 
    //                 << "("
    //                 << e.second->getTypeGraph()->stringifyTypeClean() 
    //                 << ") ";
    // }
    // std::cerr << std::endl;
    
    auto paramTypes = getTypeGraph()->getLLVMParamTypes(TheModule);
    auto resType = getTypeGraph()->getLLVMResultType(TheModule);
    paramTypes.push_back(getEnvStructType()->getPointerTo());

    auto newFuncType = llvm::FunctionType::get(resType, paramTypes, false);
    funcPrototype = llvm::Function::Create(
        newFuncType, llvm::Function::ExternalLinkage, id, TheModule
    );
}

llvm::Value *DefStmt::generateTrampoline() {
    std::cerr << "generateTrampoline() called for DefStmt\n";
    exit(1);
}
llvm::Value *Function::generateTrampoline()
{
    auto trampolineEnvMallocInst =
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
            machinePtrType, getEnvStructType(),
            llvm::ConstantExpr::getSizeOf(getEnvStructType()), nullptr,
            TheMalloc);
    auto trampolineEnvMalloc = 
        Builder.Insert(trampolineEnvMallocInst, getId() + ".envmalloc");

    //TODO(ORF): Find out how much to allocate for the trampoline
    auto trampolineMallocSize = c32(16);
    auto trampolineMallocInst = 
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
            machinePtrType, i8, 
            llvm::ConstantExpr::getSizeOf(i8), trampolineMallocSize,
            TheMalloc);
    auto trampolineMalloc = 
        Builder.Insert(trampolineMallocInst, getId() + ".trampmalloc");
    
    // fill the env struct
    int i = 0;
    llvm::Value *currEnvLoc;
    AST *currDepNode;
    for (const auto &ext: external) {
        currEnvLoc = Builder.CreateGEP(
            trampolineEnvMalloc, {c32(0), c32(i)}, "tramp.currenvloc");
        currDepNode = ext.second->getNode();
        if (inf.deepSubstitute(ext.second->getTypeGraph())->isFunction()) {
            // save a backlog of global-structloc pairs to be
            // processed after trampoline creations for mutually recursive functions
            envBacklog.push_back({currDepNode, currEnvLoc});
        } else {
            auto currDepVal = Builder.CreateLoad(
                currDepNode->getGlobalLiveValue(), "loadedglobaltmp");
            Builder.CreateStore(currDepVal, currEnvLoc, false);
        }
        i++;
    }
    llvm::Function *initTrampoline = llvm::Intrinsic::getDeclaration(
        TheModule, llvm::Intrinsic::init_trampoline);
    llvm::Function *adjustTrampoline = llvm::Intrinsic::getDeclaration(
        TheModule, llvm::Intrinsic::adjust_trampoline);
    auto *bitcastedFuncProto = Builder.CreatePointerCast(
                funcPrototype, i8->getPointerTo(), "castedfuncptrtmp"),
         *bitcastedEnvStruct = Builder.CreatePointerCast(
                trampolineEnvMalloc, i8->getPointerTo(), "castedfuncenvtmp");
        Builder.CreateCall(
            initTrampoline, 
            {trampolineMalloc, bitcastedFuncProto, bitcastedEnvStruct}
        );
    auto *adjustedTrampoline =
        Builder.CreateCall(adjustTrampoline, {trampolineMalloc}, "adjustedtrampoline");
    return Builder.CreatePointerCast(
        adjustedTrampoline, TG->getLLVMType(TheModule), "castedtrampoline"
    );

}

void DefStmt::processEnvBacklog() {
    std::cerr << "processEnvBacklog() called for DefStmt \n";
    exit(1);
}
void Function::processEnvBacklog()
{
    for (const auto &pair: envBacklog) {
        Builder.CreateStore(
            Builder.CreateLoad(pair.first->getGlobalLiveValue()),
            pair.second,
            false
        );
    }
}

void DefStmt::generateBody() {
    std::cerr << "generateBody called for DefStmt\n";
    exit(1);
}
void Function::generateBody()
{    
    // std::cerr << "Symbol " << id << " needs: ";
    // for(auto pair: external)
    // {
    //     std::cerr   << pair.first 
    //                 << "("
    //                 << inf.deepSubstitute(pair.second->getTypeGraph())->stringifyTypeClean() 
    //                 << ")- ";
    // }
    // std::cerr << std::endl;

    llvm::BasicBlock *prevBB = Builder.GetInsertBlock();
    openScopeOfAll();
    llvm::BasicBlock *newBB = llvm::BasicBlock::Create(TheContext, "entry", funcPrototype);
    Builder.SetInsertPoint(newBB);
    int i = 0;
    std::vector<std::pair<int,llvm::Value *>> previousGlobals;
    for (auto &arg : funcPrototype->args())
    {
        if ((long unsigned) i == par_list.size()) {
            arg.addAttr(llvm::Attribute::Nest);
            break; // this exits the loop after handling the 'real' args
        }
        arg.setName(par_list[i]->getId());
        LLValues.insert({par_list[i]->getId(), &arg});
        previousGlobals.push_back({i, par_list[i]->updateGlobalValue(&arg)});
        i++;
    }
    i = 0;
    auto envStruct = funcPrototype->getArg(par_list.size());
    for (auto const &ext: external) {
        // insert env loaded values in LLValues table
        auto envField = Builder.CreateLoad(
            Builder.CreateGEP(envStruct, {c32(0), c32(i)}, "envfield")
        );
        LLValues.insert({ext.first, envField});
        i++;
    }
    // for (auto const &pair: previousGlobals) {
    //     if (par_list[pair.first]->getGlobalLiveValue() == nullptr) continue;
    //     Builder.CreateStore(pair.second, par_list[pair.first]->getGlobalLiveValue(), false);
    // }
    auto retVal = expr->compile();
    for (auto const &pair: previousGlobals) {
        if (par_list[pair.first]->getGlobalLiveValue() == nullptr) continue;
        if (pair.second == nullptr) continue;
        Builder.CreateStore(pair.second, par_list[pair.first]->getGlobalLiveValue());
    }
    Builder.CreateRet(retVal);
    closeScopeOfAll();
    bool bad = llvm::verifyFunction(*funcPrototype, &llvm::errs());
    if (bad)
    {
        // again, this is an internal error most likely
        std::cerr << "Func verification failed for " << id << '\n';
        funcPrototype->print(llvm::errs(), nullptr);
        exit(1);
    }
    Builder.SetInsertPoint(prevBB);
    TheFPM->run(*funcPrototype);
}

llvm::Value *Function::compile()
{
    generateLLVMPrototype();
    generateBody();
    llvm::Value *newFunctionTrampoline = generateTrampoline();    
    processEnvBacklog();
    newFunctionTrampoline->setName(id);
    LLValues.insert({id, newFunctionTrampoline});
    updateGlobalValue(newFunctionTrampoline);
    return nullptr;
}
llvm::Value *Array::compile()
{
    // Get dimensions
    int dimensions = this->get_dimensions();

    // Create the Alloca with the correct type
    TypeGraph *containedTypeGraph = inf.deepSubstitute(T->get_TypeGraph());
    TypeGraph *arrayTypeGraph = new ArrayTypeGraph(dimensions, new RefTypeGraph(containedTypeGraph));
    llvm::Type *LLVMContainedType = containedTypeGraph->getLLVMType(TheModule);
    llvm::Type *LLVMType = arrayTypeGraph->getLLVMType(TheModule)->getPointerElementType();
    // llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, id, LLVMType);
    auto *LLVMMAllocInst = llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), machinePtrType,
                                                        LLVMType, llvm::ConstantExpr::getSizeOf(LLVMType),
                                                        nullptr,
                                                        // nullptr,
                                                        TheMalloc,
                                                        "arr.def.malloc");
    llvm::Value *LLVMMAllocStruct = Builder.Insert(LLVMMAllocInst, "arr.def.mutable");

    // Turn dimensions into a value
    llvm::ConstantInt *LLVMDimensions = c32(dimensions);

    // Get the size of each dimension
    std::vector<llvm::Value *> LLVMSize = {};
    for (auto e : expr_list)
    {
        LLVMSize.push_back(e->compile());
    }

    // Allocate correct space for one dimensional array
    llvm::Value *LLVMArraySize = nullptr;
    for (auto size : LLVMSize)
    {
        if (!LLVMArraySize)
        {
            LLVMArraySize = size;
            continue;
        }

        LLVMArraySize = Builder.CreateMul(LLVMArraySize, size, "arr.def.multmp");
    }

    llvm::Instruction *LLVMMalloc =
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
                                     machinePtrType,
                                     LLVMContainedType,
                                     llvm::ConstantExpr::getSizeOf(LLVMContainedType),
                                     LLVMArraySize,
                                     //  nullptr,
                                     TheMalloc,
                                     "arr.def.malloc");

    llvm::Value *LLVMAllocatedMemory = Builder.Insert(LLVMMalloc);

    // Assign the values to the members
    llvm::Value *arrayPtrLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(0)}, "arr.def.arrayptrloc");
    Builder.CreateStore(LLVMAllocatedMemory, arrayPtrLoc);

    llvm::Value *dimensionsLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(1)}, "arr.def.dimloc");
    Builder.CreateStore(LLVMDimensions, dimensionsLoc);

    // Step over the other fields
    int step = 2;

    // Store the sizes of the dimensions
    int sizeIndex;
    for (int i = 0; i < dimensions; i++)
    {
        sizeIndex = i + step;
        llvm::Value *sizeLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(sizeIndex)}, "arr.def.sizeloc");
        Builder.CreateStore(LLVMSize[i], sizeLoc);
    }

    // Add the array to the map
    LLVMMAllocStruct->setName(id);
    LLValues.insert({id, LLVMMAllocStruct});
    updateGlobalValue(LLVMMAllocStruct);

    return nullptr;
}
llvm::Value *Variable::compile()
{
    // Get TheFunction insert block
    //llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create the Alloca with the correct type
    llvm::Type *LLVMType = T->get_TypeGraph()->getLLVMType(TheModule);
    // llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, id, LLVMType);
    auto *LLVMMallocInst = llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), machinePtrType,
                                                        LLVMType, llvm::ConstantExpr::getSizeOf(LLVMType),
                                                        nullptr,
                                                        // nullptr,
                                                        TheMalloc,
                                                        "var.def.malloc");
    llvm::Value *LLVMMAlloc = Builder.Insert(LLVMMallocInst, "var.def.mutable");

    // Add the variable to the map
    LLVMMAlloc->setName(id);
    LLValues.insert({id, LLVMMAlloc});
    updateGlobalValue(LLVMMAlloc);

    return nullptr;
}
llvm::Value *Letdef::compile()
{
    // Not recursive
    if (!recursive)
    {
        for (auto def : def_list)
        {
            def->compile();
        }
    }
    // NOTE: Must handle let rec
    // (mutually) Recursive functions
    else
    {
        // Get all function signatures
        for (auto &func : def_list)
        {   
            if (!func->isFunctionDefinition()) {
                std::cerr << "Recursive definition is NOT a function\n";
                exit(1);
            }
            func->generateLLVMPrototype();
        }
        // Create and store all function trampolines
        for (auto &func : def_list) 
        {
            auto newFuncTrampoline = func->generateTrampoline();
            newFuncTrampoline->setName(func->getId());
            LLValues.insert({func->getId(), newFuncTrampoline});
            func->updateGlobalValue(newFuncTrampoline);
        }
        // fill functions of all trampolines
        // (necessary to support mutually recursive funcs)
        // and then compile their bodies
        for (auto &func : def_list)
        {
            func->processEnvBacklog();
            func->generateBody();
        }
    }
    return nullptr;
}
llvm::Value *Typedef::compile()
{
    return nullptr;
}
llvm::Value *Program::compile()
{
    for (auto def : definition_list)
    {
        def->compile();
    }

    return nullptr;
}

/*********************************/
/**        Expressions           */
/*********************************/

// literals
llvm::Value *String_literal::compile()
{
    // llvm::Type* str_type = llvm::ArrayType::get(i8, s.length() + 1);
    // Get TheFunction insert block
    //llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::Value *strVal = getGlobalString(s, Builder);

    int size = s.size() + 1;

    llvm::Value *LLVMArraySize = c32(size);
    // llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, "", arrCharType->getPointerElementType());
    auto *LLVMMallocInst = llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), machinePtrType,
                                                        arrCharType->getPointerElementType(),
                                                        llvm::ConstantExpr::getSizeOf(arrCharType->getPointerElementType()),
                                                        nullptr,
                                                        // nullptr,
                                                        TheMalloc,
                                                        "str.literal.malloc");
    llvm::Value *LLVMMallocStruct = Builder.Insert(LLVMMallocInst, "str.literal.mutable");

    // Allocate memory for string
    llvm::Instruction *LLVMMalloc =
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
                                     machinePtrType,
                                     i8,
                                     llvm::ConstantExpr::getSizeOf(i8),
                                     LLVMArraySize,
                                     //  nullptr,
                                     TheMalloc,
                                     "stringalloc");

    llvm::Value *LLVMAllocatedMemory = Builder.Insert(LLVMMalloc);
    // Assign the values to the members
    llvm::Value *arrayPtrLoc = Builder.CreateGEP(LLVMMallocStruct, {c32(0), c32(0)}, "stringptrloc");
    Builder.CreateStore(LLVMAllocatedMemory, arrayPtrLoc);

    llvm::Value *dimensionsLoc = Builder.CreateGEP(LLVMMallocStruct, {c32(0), c32(1)}, "dimensionsloc");
    Builder.CreateStore(c32(1), dimensionsLoc);

    llvm::Value *sizeLoc = Builder.CreateGEP(LLVMMallocStruct, {c32(0), c32(2)}, "sizeloc");
    Builder.CreateStore(LLVMArraySize, sizeLoc);

    Builder.CreateCall(TheModule->getFunction("strcpy"), {LLVMAllocatedMemory, strVal});

    return LLVMMallocStruct;
}
llvm::Value *Char_literal::compile()
{
    return c8(c);
}
llvm::Value *Bool_literal::compile()
{
    return c1(b);
}
llvm::Value *Float_literal::compile()
{
    //! Possibly wrong, make sure size and stuff are ok
    return f80(d);
}
llvm::Value *Int_literal::compile()
{
    return c32(n);
}
llvm::Value *Unit_literal::compile()
{
    //! Possibly wrong, null value of type void seems dangerous
    return unitVal();
}

llvm::Value *Literal::LLVMCompare(llvm::Value *V)
{
    return nullptr;
}
llvm::Value *Char_literal::LLVMCompare(llvm::Value *V)
{
    llvm::Value *literalV = compile();
    return Builder.CreateICmpEQ(literalV, V, "literal.char.compare");
}
llvm::Value *Int_literal::LLVMCompare(llvm::Value *V)
{
    llvm::Value *literalV = compile();
    return Builder.CreateICmpEQ(literalV, V, "literal.int.compare");
}
llvm::Value *Float_literal::LLVMCompare(llvm::Value *V)
{
    llvm::Value *literalV = compile();
    return Builder.CreateFCmpOEQ(literalV, V, "literal.char.compare");
}

// Operators
llvm::Value *BinOp::compile()
{
    if (op == T_dblampersand || op == T_dblbar)
    {
        auto *lhsLogicVal = lhs->compile();
        auto *shortCircuitExitBB = llvm::BasicBlock::Create(
                 TheContext, "shortcircuit.exit", Builder.GetInsertBlock()->getParent()),
             *shortCircuitImpossibleBB = llvm::BasicBlock::Create(
                 TheContext, "shortcircuit.impossible", Builder.GetInsertBlock()->getParent());
        llvm::IRBuilder<> TmpB(TheContext);
        TmpB.SetInsertPoint(shortCircuitExitBB);
        auto *phiCollector = TmpB.CreatePHI(i1, 2, "shortcircuit.restmp");

        if (op == T_dblampersand)
        {
            auto *decider = Builder.CreateICmpEQ(lhsLogicVal, c1(false), "shortcircuit.andtmp");
            Builder.CreateCondBr(decider, shortCircuitExitBB, shortCircuitImpossibleBB);
            phiCollector->addIncoming(c1(false), Builder.GetInsertBlock());
            Builder.SetInsertPoint(shortCircuitImpossibleBB);
            auto *rhsLogicVal = rhs->compile();
            auto *operationRes = Builder.CreateAnd(lhsLogicVal, rhsLogicVal, "and.restmp");
            Builder.CreateBr(shortCircuitExitBB);
            phiCollector->addIncoming(operationRes, Builder.GetInsertBlock());
            Builder.SetInsertPoint(shortCircuitExitBB);
            return phiCollector;
        }
        else if (op == T_dblbar)
        {
            auto *decider = Builder.CreateICmpEQ(lhsLogicVal, c1(true), "shortcircuit.ortmp");
            Builder.CreateCondBr(decider, shortCircuitExitBB, shortCircuitImpossibleBB);
            phiCollector->addIncoming(c1(true), Builder.GetInsertBlock());
            Builder.SetInsertPoint(shortCircuitImpossibleBB);
            auto *rhsLogicVal = rhs->compile();
            auto *operationRes = Builder.CreateOr(lhsLogicVal, rhsLogicVal, "or.restmp");
            Builder.CreateBr(shortCircuitExitBB);
            phiCollector->addIncoming(operationRes, Builder.GetInsertBlock());
            Builder.SetInsertPoint(shortCircuitExitBB);
            return phiCollector;
        }
    }
    else
    {
        auto lhsVal = lhs->compile(),
             rhsVal = rhs->compile();
        auto tempTypeGraph = inf.deepSubstitute(lhs->get_TypeGraph());

        switch (op)
        {
        case '+':
            return Builder.CreateAdd(lhsVal, rhsVal, "int.addtmp");
        case '-':
            return Builder.CreateSub(lhsVal, rhsVal, "int.subtmp");
        case '*':
            return Builder.CreateMul(lhsVal, rhsVal, "int.multmp");
        case '/':
            return Builder.CreateSDiv(lhsVal, rhsVal, "int.divtmp");
        case T_mod:
            return Builder.CreateSRem(lhsVal, rhsVal, "int.modtmp");

        case T_plusdot:
            return Builder.CreateFAdd(lhsVal, rhsVal, "float.addtmp");
        case T_minusdot:
            return Builder.CreateFSub(lhsVal, rhsVal, "float.subtmp");
        case T_stardot:
            return Builder.CreateFMul(lhsVal, rhsVal, "float.multmp");
        case T_slashdot:
            return Builder.CreateFDiv(lhsVal, rhsVal, "float.divtmp");
        // for below to work, link against lib.so with -lm flag.
        case T_dblstar:
        {
            // return Builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhsVal, rhsVal, nullptr, "float.powtmp");
            return Builder.CreateCall(TheModule->getFunction("pow.custom"), {lhsVal, rhsVal}, "float.powtmp");
        }
        case T_dblbar:
            return Builder.CreateOr({lhsVal, rhsVal});
        case T_dblampersand:
            return Builder.CreateAnd({lhsVal, rhsVal});

        case '=':
        { // structural equality, they contain the same values
            return equalityHelper(lhsVal, rhsVal, tempTypeGraph, true, Builder);
        }
        case T_lessgreater:
        { // structural inequality, they do not contain the same values
            return Builder.CreateNot(equalityHelper(lhsVal, rhsVal, tempTypeGraph, true, Builder));
        }
        case T_dbleq:
        { // natural equality, they are the same object
            return equalityHelper(lhsVal, rhsVal, tempTypeGraph, false, Builder);
        }
        case T_exclameq:
        { // natural equality, they are not the same object
            return Builder.CreateNot(equalityHelper(lhsVal, rhsVal, tempTypeGraph, false, Builder));
        }
        case '<':
        {
            if (tempTypeGraph->isFloat())
            {
                // QNAN in docs is a 'quiet NaN'
                //! might need to check for QNAN btw
                return Builder.CreateFCmpOLT(lhsVal, rhsVal, "float.cmplttmp");
            }
            else
            { // int or char (which is an int too)
                return Builder.CreateICmpSLT(lhsVal, rhsVal, "int.cmplttmp");
            }
        }
        case '>':
        {
            if (tempTypeGraph->isFloat())
            {
                // QNAN in docs is a 'quiet NaN'
                //! might need to check for QNAN btw
                return Builder.CreateFCmpOGT(lhsVal, rhsVal, "float.cmpgttmp");
            }
            else
            { // int or char (which is an int too)
                return Builder.CreateICmpSGT(lhsVal, rhsVal, "int.cmpgttmp");
            }
        }
        case T_leq:
        {
            if (tempTypeGraph->isFloat())
            {
                // QNAN in docs is a 'quiet NaN'
                //! might need to check for QNAN btw
                return Builder.CreateFCmpOLE(lhsVal, rhsVal, "float.cmpletmp");
            }
            else
            { // int or char (which is an int too)
                return Builder.CreateICmpSLE(lhsVal, rhsVal, "int.cmpletmp");
            }
        }
        case T_geq:
        {
            if (tempTypeGraph->isFloat())
            {
                // QNAN in docs is a 'quiet NaN'
                //! might need to check for QNAN btw
                return Builder.CreateFCmpOGE(lhsVal, rhsVal, "float.cmpgetmp");
            }
            else
            { // int or char (which is an int too)
                return Builder.CreateICmpSGE(lhsVal, rhsVal, "int.cmpgetmp");
            }
        }
        case T_coloneq:
        {
            Builder.CreateStore(rhsVal, lhsVal);
            return unitVal();
        }
        case ';':
            return rhsVal;
        default:
            return nullptr;
        }
    }

    return nullptr;
}
llvm::Value *UnOp::compile()
{
    auto exprVal = expr->compile();

    switch (op)
    {
    case '+':
        return exprVal;
    case '-':
        return Builder.CreateSub(c32(0), exprVal, "int.negtmp");
    case T_plusdot:
        return exprVal;
    case T_minusdot:
        return Builder.CreateFSub(llvm::ConstantFP::getZeroValueForNegation(flt), exprVal, "float.negtmp");
    case T_not:
        return Builder.CreateNot(exprVal, "bool.nottmp");
    case '!':
        return Builder.CreateLoad(exprVal, "ptr.dereftmp");
    case T_delete:
    {
#ifdef LIBGC
        llvm::Instruction *i8PtrCast = llvm::CastInst::CreatePointerCast(exprVal, i8->getPointerTo(), "delete.cast", Builder.GetInsertBlock());
        Builder.CreateCall(TheModule->getFunction("GC_free"), {i8PtrCast});
#else
        Builder.Insert(llvm::CallInst::CreateFree(exprVal, Builder.GetInsertBlock()));
#endif // LIBGC
        return unitVal();
    }
    default:
        return nullptr;
    }
}

// Misc

llvm::Value *LetIn::compile()
{
    llvm::Value *retVal;
    openScopeOfAll();
    letdef->compile();
    retVal = expr->compile();
    closeScopeOfAll();
    return retVal;
}
llvm::Value *New::compile()
{
    TypeGraph *newTypeGraph = inf.deepSubstitute(TG);
    const std::string instrName = "new_" + newTypeGraph->stringifyTypeClean() + "_alloc";
    llvm::Type *newType = newTypeGraph->getContainedType()->getLLVMType(TheModule);

    llvm::Instruction *LLVMMalloc =
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
                                     machinePtrType,
                                     newType,
                                     llvm::ConstantExpr::getSizeOf(newType),
                                     nullptr,
                                     //  nullptr,
                                     TheUncollectableMalloc,
                                     instrName);

    llvm::Value *LLVMAllocatedMemory = Builder.Insert(LLVMMalloc);

    return LLVMAllocatedMemory;
}
llvm::Value *While::compile()
{
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create Basic Block for loop, body, end
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(TheContext, "whileloop");
    llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(TheContext, "whilebody");
    llvm::BasicBlock *FinishBB = llvm::BasicBlock::Create(TheContext, "whileend");

    Builder.CreateBr(LoopBB);

    /*************** LOOP ***************/

    TheFunction->getBasicBlockList().push_back(LoopBB);
    Builder.SetInsertPoint(LoopBB);

    // Emit code for the condition
    llvm::Value *LLVMCond = cond->compile();

    // Check whether to continue or finish
    //LLVMCond = Builder.CreateICmpEQ(LLVMCond, c1(true), "whileloopcheck");
    Builder.CreateCondBr(LLVMCond, BodyBB, FinishBB);

    /*************** BODY ***************/

    TheFunction->getBasicBlockList().push_back(BodyBB);
    Builder.SetInsertPoint(BodyBB);

    // Emit code for the body
    body->compile();

    // Loop
    Builder.CreateBr(LoopBB);

    /*************** FINISH ***************/

    TheFunction->getBasicBlockList().push_back(FinishBB);
    Builder.SetInsertPoint(FinishBB);

    return unitVal();
}
llvm::Value *For::compile()
{
    bool increment = (step == "to");
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
    llvm::BasicBlock *PreheaderBB = Builder.GetInsertBlock();

    // Create Basic Block for loop, body, end
    llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(TheContext, "forloop");
    llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(TheContext, "forbody");
    llvm::BasicBlock *FinishBB = llvm::BasicBlock::Create(TheContext, "forend");

    /*************** INITIALISATION ***************/

    // Emit code for start and finish values
    llvm::Value *StartV = start->compile();
    llvm::Value *FinishV = finish->compile();

    // Create value that holds the step
    llvm::Value *StepV = increment ? c32(1) : c32(-1);

    // Create scope for loop variable
    openScopeOfAll();

    // Begin loop
    Builder.CreateBr(LoopBB);

    /*************** LOOP ***************/

    TheFunction->getBasicBlockList().push_back(LoopBB);
    Builder.SetInsertPoint(LoopBB);

    // Create phi node, add an entry for start and insert to the table
    llvm::PHINode *LoopVariable = Builder.CreatePHI(i32, 2, id);
    LoopVariable->addIncoming(StartV, PreheaderBB);
    LoopVariable->setName(id);
    LLValues.insert({id, LoopVariable});
    updateGlobalValue(LoopVariable);

    // Check whether the condition is satisfied
    llvm::Value *LLVMCond =
        increment ? Builder.CreateICmpSLE(LoopVariable, FinishV, "forloopchecklte")
                  : Builder.CreateICmpSGE(LoopVariable, FinishV, "forlookcheckgte");
    Builder.CreateCondBr(LLVMCond, BodyBB, FinishBB);

    /*************** BODY ***************/

    TheFunction->getBasicBlockList().push_back(BodyBB);
    Builder.SetInsertPoint(BodyBB);

    // Emit code for the body
    body->compile();

    // Add step to the loop variable
    llvm::Value *NextV = Builder.CreateAdd(LoopVariable, StepV, "forstep");

    // Add entry to the phi node for backedge
    LoopVariable->addIncoming(NextV, Builder.GetInsertBlock());

    // Loop
    Builder.CreateBr(LoopBB);

    /*************** FINISH ***************/

    TheFunction->getBasicBlockList().push_back(FinishBB);
    Builder.SetInsertPoint(FinishBB);

    closeScopeOfAll();
    return unitVal();
}
llvm::Value *If::compile()
{
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Get return type of if
    llvm::Type *LLVMIfReturnType = TG->getLLVMType(TheModule);

    // Emit code for the condition compare
    llvm::Value *LLVMCond = cond->compile();
    LLVMCond = Builder.CreateICmpEQ(LLVMCond, c1(true), "ifcond");

    // Create blocks for the then and else cases.
    // Insert the 'then' block at the end of the function.
    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "then");
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

    // Create branch
    Builder.CreateCondBr(LLVMCond, ThenBB, ElseBB);

    // Handle then
    TheFunction->getBasicBlockList().push_back(ThenBB);
    Builder.SetInsertPoint(ThenBB);
    llvm::Value *ThenV = body->compile();
    ThenBB = Builder.GetInsertBlock();
    Builder.CreateBr(MergeBB);

    // Handle else whether it exists or not
    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);
    llvm::Value *ElseV = unitVal();
    if (else_body)
        ElseV = else_body->compile();
    ElseBB = Builder.GetInsertBlock();
    Builder.CreateBr(MergeBB);

    // Finish
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);

    llvm::PHINode *retVal = Builder.CreatePHI(LLVMIfReturnType, 2, "ifretval");
    retVal->addIncoming(ThenV, ThenBB);
    retVal->addIncoming(ElseV, ElseBB);

    return retVal;
}
llvm::Value *Dim::compile()
{
    // Step over the other fields
    int step = 2;

    // Calculate the selected dimension and make it zero based
    int selectedDim = dim->get_int() - 1;

    // Get the pointer to the array struct
    llvm::Value *LLVMPointerToStruct = LLValues[id];

    //
    llvm::Value *LLVMSizeLoc = Builder.CreateGEP(LLVMPointerToStruct, {c32(0), c32(selectedDim + step)}, "dimsizeloc");
    return Builder.CreateLoad(LLVMSizeLoc);
}
llvm::Value *ConstantCall::compile()
{
    return LLValues[id];
}
llvm::Value *FunctionCall::compile()
{
    llvm::Value *tempFunc = LLValues[id]; // this'll be a Function, due to sem (hopefully)
    std::vector<llvm::Value *> argsGiven;
    for (auto &arg : expr_list)
    {
        argsGiven.push_back(arg->compile());
    }
    return Builder.CreateCall(tempFunc, argsGiven, "func.calltmp");
}
llvm::Value *ConstructorCall::compile()
{
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Get the enum of this constructor in the custom type
    int constrIndex = constructorTypeGraph->getIndex();

    // Create the struct that will be saved in the second field of the custom type struct
    llvm::StructType *constrType = constructorTypeGraph->getLLVMType(TheModule);
    llvm::AllocaInst *LLVMContainedStructAlloca = CreateEntryBlockAlloca(TheFunction, "constructorstruct", constrType);

    // Get the custom type of this constructor
    llvm::StructType *customType = llvm::dyn_cast<llvm::StructType>(constructorTypeGraph->getCustomType()->getLLVMType(TheModule)->getPointerElementType());
    //llvm::AllocaInst *LLVMCustomStructAlloca = CreateEntryBlockAlloca(TheFunction, "customstruct", customType);

    auto LLVMCustomStructMallocInst = llvm::CallInst::CreateMalloc(
        Builder.GetInsertBlock(),
        machinePtrType,
        customType,
        llvm::ConstantExpr::getSizeOf(customType),
        nullptr,
        // nullptr,
        TheMalloc,
        "customstruct.malloc");
    llvm::Value *LLVMCustomStructPtr = Builder.Insert(LLVMCustomStructMallocInst);

    // Codegen and store the parameters to its fields
    llvm::Value *constrFieldLoc, *LLVMParam;
    for (int i = 0; i < (int)expr_list.size(); i++)
    {
        LLVMParam = expr_list[i]->compile();

        constrFieldLoc = Builder.CreateGEP(LLVMContainedStructAlloca, {c32(0), c32(i)}, "constrFieldLoc");
        Builder.CreateStore(LLVMParam, constrFieldLoc);
    }

    // Store the enum into custom struct
    llvm::Value *enumLoc = Builder.CreateGEP(LLVMCustomStructPtr, {c32(0), c32(0)}, "customenumloc");
    Builder.CreateStore(c32(constrIndex), enumLoc);

    // Get the expected field type of the custom type
    llvm::Type *customFieldTypePtr = customType->getTypeAtIndex(1)->getPointerTo();

    // Bitcast the constructor struct into the custom struct
    llvm::Instruction *LLVMCastStructPtr = llvm::CastInst::CreatePointerCast(LLVMContainedStructAlloca, customFieldTypePtr, "constructorbitcast", Builder.GetInsertBlock());
    //llvm::Value *LLVMCastStructPtr = LLVMBitCast->getOperand(0);

    // Store it into the custom struct
    llvm::Value *LLVMCastStruct = Builder.CreateLoad(LLVMCastStructPtr);
    llvm::Value *constructorLoc = Builder.CreateGEP(LLVMCustomStructPtr, {c32(0), c32(1)}, "customconstructorloc");
    Builder.CreateStore(LLVMCastStruct, constructorLoc);

    return LLVMCustomStructPtr;
}
llvm::Value *ArrayAccess::compile()
{
    // Emit code to calculate indices
    std::vector<llvm::Value *> LLVMArrayIndices = {};
    for (auto e : expr_list)
    {
        LLVMArrayIndices.push_back(e->compile());
    }

    // Get the complete array struct as an alloca
    llvm::Value *LLVMArrayStruct = LLValues[id];
    std::vector<llvm::Value *> LLVMSize = {};

    // Load necessary values
    llvm::Value *arrayPtrLoc = Builder.CreateGEP(LLVMArrayStruct, {c32(0), c32(0)}, "arr.acc.ptrloc");
    llvm::Value *LLVMArray = Builder.CreateLoad(arrayPtrLoc);

    // Step over the other fields
    int step = 2;

    // Get all the sizes of the dimensions
    llvm::Value *sizeLoc;
    int sizeIndex, dimensions = expr_list.size();
    for (int i = 0; i < dimensions; i++)
    {
        sizeIndex = i + step;
        sizeLoc = Builder.CreateGEP(LLVMArrayStruct, {c32(0), c32(sizeIndex)}, "arr.acc.sizeloc");
        LLVMSize.push_back(Builder.CreateLoad(sizeLoc));
    }

    //Bounds check
    llvm::BasicBlock *currentBB = llvm::BasicBlock::Create(TheContext, "boundcheck.init", Builder.GetInsertBlock()->getParent()),
                     *outOfBoundsBB = llvm::BasicBlock::Create(TheContext, "boundcheck.outofbounds", Builder.GetInsertBlock()->getParent());
    Builder.CreateBr(currentBB);
    for (int i = 0; i < dimensions; i++)
    {
        Builder.SetInsertPoint(currentBB);
        auto checkDimVal = 
            Builder.CreateICmpSLT(LLVMArrayIndices[i], LLVMSize[i], std::string("checkdim.") + std::to_string(i));
        auto nextBB = llvm::BasicBlock::Create(TheContext, "boundcheck.nextdim", Builder.GetInsertBlock()->getParent());
        Builder.CreateCondBr(checkDimVal, nextBB, outOfBoundsBB);
        currentBB = nextBB;
    }
    Builder.SetInsertPoint(outOfBoundsBB);
    Builder.CreateCall(TheModule->getFunction("writeString"),
                    {getGlobalString("Runtime error: array index out of bounds", Builder)});
    Builder.CreateCall(TheModule->getFunction("_exit"), {c32(1)});
    Builder.CreateBr(outOfBoundsBB);

    Builder.SetInsertPoint(currentBB);

    // Calculate the position of the requested element
    // in the one dimensional representation of the array.
    llvm::Value *LLVMTemp = LLVMArrayIndices[dimensions - 1],
                *LLVMArrayLoc = LLVMTemp,
                *LLVMSuffixSizeMul = LLVMSize[dimensions - 1];
    for (int i = dimensions - 2; i >= 0; i--)
    {
        // Multiply SuffixSizeMul with the current index
        LLVMTemp = Builder.CreateMul(LLVMSuffixSizeMul, LLVMArrayIndices[i]);

        // Add the result to the total location
        LLVMArrayLoc = Builder.CreateAdd(LLVMArrayLoc, LLVMTemp);

        // Multiplication not needed the last time
        if (i != 0)
        {
            // Multiply SuffixSizeMul with the size of current dimension
            LLVMSuffixSizeMul = Builder.CreateMul(LLVMSuffixSizeMul, LLVMSize[i]);
        }
    }

    return Builder.CreateGEP(LLVMArray, LLVMArrayLoc, "arr.acc.elemptr");
}

// Match

llvm::Value *Match::compile()
{
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Emit code for expression to be matched
    llvm::Value *toMatchV = toMatch->compile();

    // Basic Block to exit the match
    llvm::BasicBlock *FinishBB = llvm::BasicBlock::Create(TheContext, "match.finish");

    // One basic block for every clause
    std::vector<llvm::BasicBlock *> ClauseBB = {};

    // One value for each clause (for the phi node)
    std::vector<llvm::Value *> ClauseV = {};

    // Create first basic block
    llvm::BasicBlock *SuccessBB;
    llvm::BasicBlock *NextClauseBB = llvm::BasicBlock::Create(TheContext, "match.firstclause");
    Builder.CreateBr(NextClauseBB);
    for (int i = 0; i < (int)clause_list.size(); i++)
    {
        auto c = clause_list[i];

        /*************** CHECK CLAUSE ***************/
        TheFunction->getBasicBlockList().push_back(NextClauseBB);
        Builder.SetInsertPoint(NextClauseBB);

        // Create the two possible next basic blocks
        bool isLastClause = (i == (int)clause_list.size() - 1);
        std::string NextClauseBBName = (isLastClause) ? "match.fail" : "match.nextclause";
        NextClauseBB = llvm::BasicBlock::Create(TheContext, NextClauseBBName);
        SuccessBB = llvm::BasicBlock::Create(TheContext, "match.success");

        // Checks whether it matched, possibly adds to LLValues
        openScopeOfAll();
        llvm::Value *tryToMatchV = c->tryToMatch(toMatchV, NextClauseBB);

        // Branch to correct basic block
        Builder.CreateCondBr(tryToMatchV, SuccessBB, NextClauseBB);

        /*************** MATCH SUCCESS ***************/
        TheFunction->getBasicBlockList().push_back(SuccessBB);
        Builder.SetInsertPoint(SuccessBB);

        // Emit code for the expression of the clause and save it
        ClauseV.push_back(c->compile());

        closeScopeOfAll();

        // Save the current basic block for the phi node
        ClauseBB.push_back(Builder.GetInsertBlock());

        // Finish matching
        Builder.CreateBr(FinishBB);
    }

    /*************** NO MATCH ***************/
    TheFunction->getBasicBlockList().push_back(NextClauseBB);
    Builder.SetInsertPoint(NextClauseBB);
    Builder.CreateCall(TheModule->getFunction("writeString"),
                       {getGlobalString("Runtime Error: No clause matches given expression\n", Builder)});
    Builder.CreateCall(TheModule->getFunction("_exit"), {c32(1)});

    // Never going to get here but llvm complains anyway
    Builder.CreateBr(NextClauseBB);

    /*************** FINISH ***************/
    TheFunction->getBasicBlockList().push_back(FinishBB);
    Builder.SetInsertPoint(FinishBB);

    // Create phi node and add all the incoming values
    llvm::Type *retType = TG->getLLVMType(TheModule);
    llvm::PHINode *retVal = Builder.CreatePHI(retType, clause_list.size(), "match.retval");

    for (int i = 0; i < (int)clause_list.size(); i++)
    {
        retVal->addIncoming(ClauseV[i], ClauseBB[i]);
    }

    return retVal;
}
llvm::Value *Clause::compile()
{
    return expr->compile();
}
llvm::Value *Clause::tryToMatch(llvm::Value *toMatchV, llvm::BasicBlock *NextClauseBB)
{
    // Give the value to be matched to the correct pattern
    pattern->set_toMatchV(toMatchV);

    // Give the correct basic block to branch in case of failure
    pattern->set_NextClauseBB(NextClauseBB);

    // Find out whether the match was successful
    return pattern->compile();
}

void Pattern::set_toMatchV(llvm::Value *v)
{
    toMatchV = v;
}
void Pattern::set_NextClauseBB(llvm::BasicBlock *b)
{
    NextClauseBB = b;
}
llvm::Value *PatternLiteral::compile()
{

    return literal->LLVMCompare(toMatchV);
}
llvm::Value *PatternId::compile()
{
    // Add a variable with this value to the table
    toMatchV->setName(id);
    LLValues.insert({id, toMatchV});
    updateGlobalValue(toMatchV);

    // Match was successful
    return c1(true);
}
llvm::Value *PatternConstr::compile()
{
    // Create alloca and store toMatchV to it
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
    //llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, "pattern.constr.alloca", toMatchV->getType());
    //Builder.CreateStore(toMatchV, LLVMAlloca);

    // Check whether they were created by the same constructor
    // if not then move to the next clause of the match
    int index = constrTypeGraph->getIndex();
    llvm::Value *toMatchIndexLoc = Builder.CreateGEP(toMatchV, {c32(0), c32(0)});
    llvm::Value *toMatchIndex = Builder.CreateLoad(toMatchIndexLoc, "pattern.constr.loadindex");
    llvm::Value *indexCmp = Builder.CreateICmpEQ(c32(index), toMatchIndex);
    llvm::BasicBlock *SameConstrBB = llvm::BasicBlock::Create(TheContext, "pattern.constr.sameconstr");
    Builder.CreateCondBr(indexCmp, SameConstrBB, NextClauseBB);

    // We know that they come from the same constructor now
    TheFunction->getBasicBlockList().push_back(SameConstrBB);
    Builder.SetInsertPoint(SameConstrBB);

    // Get the actual type of the struct of the constructor
    llvm::StructType *constrType = constrTypeGraph->getLLVMType(TheModule);
    llvm::Type *constrTypePtr = constrType->getPointerTo();

    // Create alloca with the struct of the constructor
    llvm::Value *toMatchConstrStructLoc = Builder.CreateGEP(toMatchV, {c32(0), c32(1)});
    // llvm::Value *toMatchConstrStruct = Builder.CreateLoad(toMatchConstrStructLoc, "pattern.constr.loadconstrstruct");
    // llvm::AllocaInst *toMatchConstrStructAlloca = CreateEntryBlockAlloca(TheFunction, "pattern.constr.constrstructalloca", toMatchConstrStruct->getType());
    // Builder.CreateStore(toMatchConstrStruct, toMatchConstrStructAlloca);

    // Bitcast the second field of the toMatchV struct to constrTypePtr
    llvm::Value *LLVMCastStructPtr = llvm::CastInst::CreatePointerCast(toMatchConstrStructLoc, constrTypePtr, "pattern.constr.bitcast", Builder.GetInsertBlock());
    //llvm::Value *LLVMCastStructPtr = LLVMBitCast->getOperand(0);

    // This value will be used to check that all fields can be matched
    llvm::Value *canMatchFields = c1(true);

    // Recursively try to match the fields
    for (int i = 0; i < (int)pattern_list.size(); i++)
    {
        auto p = pattern_list[i];

        // Get the field and try to match
        llvm::Value *castStructFieldLoc = Builder.CreateGEP(LLVMCastStructPtr, {c32(0), c32(i)}, "pattern.constr.fieldloc");
        llvm::Value *tempV = Builder.CreateLoad(castStructFieldLoc, "pattern.constr.structfield");
        p->set_toMatchV(tempV);
        p->set_NextClauseBB(NextClauseBB);
        tempV = p->compile();

        // Use and instruction to ensure that all matches succeed
        canMatchFields = Builder.CreateAnd(canMatchFields, tempV);
    }

    return canMatchFields;
}
