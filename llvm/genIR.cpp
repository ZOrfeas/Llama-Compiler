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
#include "lexer.hpp" // to get extern yylineno and not crash from ast include
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
    ~LLTable() {}
};
LLTable<llvm::Value *> LLValues;
// this can help with the function pointer stuff (member function getType())
// LLTable<llvm::Function*> LLFunctions;
// LLTable<llvm::AllocaInst> LLAllocas;

void openScopeOfAll()
{
    LLValues.openScope();
    // LLFunctions.openScope();
    // LLAllocas.openScope();
}
void closeScopeOfAll()
{
    LLValues.closeScope();
    // LLFunctions.closeScope();
    // LLAllocas.closeScope();
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

/*********************************/
/**       Initializations        */
/*********************************/

llvm::LLVMContext AST::TheContext;
llvm::IRBuilder<> AST::Builder(AST::TheContext);
llvm::Module *AST::TheModule;
llvm::legacy::FunctionPassManager *AST::TheFPM;

llvm::Type *AST::i1;
llvm::Type *AST::i8;
llvm::Type *AST::i32;
llvm::Type *AST::flt;
llvm::Type *AST::unitType;
llvm::Type *AST::machinePtrType;
llvm::Type *AST::arrCharType;

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
    i1 = type_bool->getLLVMType(TheModule);
    i8 = type_char->getLLVMType(TheModule);
    i32 = type_int->getLLVMType(TheModule);
    flt = type_float->getLLVMType(TheModule);
    unitType = type_unit->getLLVMType(TheModule);
    machinePtrType = llvm::Type::getIntNTy(TheContext, TheModule->getDataLayout().getMaxPointerSizeInBits());
    arrCharType = (new ArrayTypeGraph(1, new RefTypeGraph(type_char)))->getLLVMType(TheModule);
    std::vector<std::pair<std::string, llvm::Function*>> *libFunctions = genLibGlueLogic();
    for (auto &libFunc: *libFunctions) { LLValues.insert(libFunc); }
    llvm::FunctionType *main_type = llvm::FunctionType::get(i32, {}, false);
    llvm::Function *main =
        llvm::Function::Create(main_type, llvm::Function::ExternalLinkage,
                               "main", TheModule);
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", main);
// TODO: Initilize lib functions here
    Builder.SetInsertPoint(BB);
    compile(); // compile the program code
    // Below means that each Function codegen is responsible for restoring insert point
    Builder.CreateRet(c32(0));
    bool bad = llvm::verifyModule(*TheModule, &llvm::errs());
    if (bad)
    { // internal error
        std::cerr << "The IR is bad!" << std::endl;
        TheModule->print(llvm::errs(), nullptr);
        std::exit(1);
    }
    // this means we would probably need to optimize each function seperately
    TheFPM->run(*main);
}
void AST::printLLVMIR()
{
    TheModule->print(llvm::outs(), nullptr);
}

/*********************************/
/**        Definitions           */
/*********************************/

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
    llvm::Value *exprVal = expr->compile();
    exprVal->setName(id);
    LLValues.insert({id, exprVal});
    return nullptr;
}
llvm::Value *Function::compile()
{
    llvm::BasicBlock *prevBB = Builder.GetInsertBlock();
    llvm::Function *newFunction;
    if (llvm::FunctionType *newFuncType =
            llvm::dyn_cast<llvm::FunctionType>(TG->getLLVMType(TheModule)->getPointerElementType()))
    {
        newFunction = llvm::Function::Create(newFuncType, llvm::Function::ExternalLinkage,
                                             id, TheModule);
    }
    else
    {
        std::cout << "Internal error, FunctionType dyn_cast failed\n";
        exit(1);
    }
    LLValues.insert({id, newFunction});
    openScopeOfAll();
    llvm::BasicBlock *newBB = llvm::BasicBlock::Create(TheContext, "entry", newFunction);
    Builder.SetInsertPoint(newBB);
    int i = 0;
    for (auto &arg : newFunction->args())
    {
        arg.setName(par_list[i]->getId());
        LLValues.insert({par_list[i]->getId(), &arg});
        i++;
    }
    Builder.CreateRet(expr->compile());
    closeScopeOfAll();
    bool bad = llvm::verifyFunction(*newFunction);
    // again, this is an internal error most likely
    if (bad) { std::cerr << "Func verification failed for "<< id <<'\n'; newFunction->print(llvm::errs(), nullptr); exit(1); }
    Builder.SetInsertPoint(prevBB);
    TheFPM->run(*newFunction);
    return nullptr; // doesn't matter what it returns, its a definition not an expression
}
llvm::Value *Array::compile()
{
    // Get TheFunction insert block
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

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
                                                        nullptr, nullptr, "arr.def.malloc");
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
                                     nullptr,
                                     "arr.def.malloc"
                                    );

    llvm::Value *LLVMAllocatedMemory = Builder.Insert(LLVMMalloc);

    // Assign the values to the members
    llvm::Value *arrayPtrLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(0)}, "arr.def.arrayptrloc"); 
    Builder.CreateStore(LLVMAllocatedMemory, arrayPtrLoc);

    llvm::Value *dimensionsLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(1)}, "arr.def.dimloc"); 
    Builder.CreateStore(LLVMDimensions, dimensionsLoc);

    int sizeIndex;
    for (int i = 0; i < dimensions; i++)
    {
        sizeIndex = i + 2;
        llvm::Value *sizeLoc = Builder.CreateGEP(LLVMMAllocStruct, {c32(0), c32(sizeIndex)}, "arr.def.sizeloc");
        Builder.CreateStore(LLVMSize[i], sizeLoc);
    }

    // Add the array to the map
    LLValues.insert({id, LLVMMAllocStruct});

    return nullptr;
}
llvm::Value *Variable::compile()
{
    // Get TheFunction insert block
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create the Alloca with the correct type
    llvm::Type *LLVMType = T->get_TypeGraph()->getLLVMType(TheModule);
    // llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, id, LLVMType);
    auto *LLVMMallocInst = llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), machinePtrType,
                                                        LLVMType, llvm::ConstantExpr::getSizeOf(LLVMType),
                                                        nullptr, nullptr, "var.def.malloc");
    llvm::Value *LLVMMAlloc = Builder.Insert(LLVMMallocInst, "var.def.mutable");

    // Add the variable to the map
    LLValues.insert({id, LLVMMAlloc});

    return nullptr;
}
llvm::Value *Letdef::compile()
{   
    // Not recursive
    if(!recursive)
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

        // Compile bodies
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
}

/*********************************/
/**        Expressions           */
/*********************************/

// literals

llvm::Value *String_literal::compile()
{
    // llvm::Type* str_type = llvm::ArrayType::get(i8, s.length() + 1);
    // Get TheFunction insert block
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
    llvm::Value *strVal = Builder.CreateGlobalStringPtr(s);
    int size = s.size() + 1;
    
    llvm::Value *LLVMArraySize = c32(size);
    // llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, "", arrCharType->getPointerElementType());
    auto *LLVMMallocInst = llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), machinePtrType,
                                                        arrCharType->getPointerElementType(),
                                                        llvm::ConstantExpr::getSizeOf(arrCharType->getPointerElementType()),
                                                        nullptr, nullptr, "str.literal.malloc");
    llvm::Value *LLVMMallocStruct = Builder.Insert(LLVMMallocInst, "str.literal.mutable");

    // Allocate memory for string
    llvm::Instruction *LLVMMalloc =
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(),
                                     machinePtrType,
                                     i8,
                                     llvm::ConstantExpr::getSizeOf(i8),
                                     LLVMArraySize,
                                     nullptr,
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

    // // Copy the string
    // for(unsigned int i = 0; i < s.size(); i++)
    // {
    //     llvm::Value *stringElemLoc = Builder.CreateGEP(LLVMAllocatedMemory, {c32(i)}, "stringelemloc");
    //     Builder.CreateStore(c8(s[i]), stringElemLoc);
    // }

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

// Operators

llvm::Value *BinOp::compile()
{
    auto lhsVal = lhs->compile(),
         rhsVal = rhs->compile();
    auto tempTypeGraph = inf.deepSubstitute(lhs->get_TypeGraph());

    switch (op)
    {
    case '+': return Builder.CreateAdd(lhsVal, rhsVal, "int.addtmp");
    case '-': return Builder.CreateSub(lhsVal, rhsVal, "int.subtmp");
    case '*': return Builder.CreateMul(lhsVal, rhsVal, "int.multmp");
    case '/': return Builder.CreateSDiv(lhsVal, rhsVal, "int.divtmp");
    case T_mod: return Builder.CreateSRem(lhsVal, rhsVal, "int.modtmp");
    
    case T_plusdot: return Builder.CreateFAdd(lhsVal, rhsVal, "float.addtmp");
    case T_minusdot: return Builder.CreateFSub(lhsVal, rhsVal, "float.subtmp");
    case T_stardot: return Builder.CreateFMul(lhsVal, rhsVal, "float.multmp");
    case T_slashdot: return Builder.CreateFDiv(lhsVal, rhsVal, "float.divtmp");
    // for below to work, link against lib.so with -lm flag.
    case T_dblstar: return Builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhsVal, rhsVal, nullptr, "float.powtmp");

    case T_dblbar: return Builder.CreateOr({lhsVal, rhsVal});
    case T_dblampersand: return Builder.CreateAnd({lhsVal, rhsVal});

    case '=':
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: structural equality, if struct here check fields recursively
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpOEQ(lhsVal, rhsVal, "float.cmpeqtmp");
        } else{ // anything else (that passed sem)
            return Builder.CreateICmpEQ(lhsVal, rhsVal, "int.cmpeqtmp");
        }
    }
    case T_lessgreater:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: structural inequality
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpONE(lhsVal, rhsVal, "float.cmpnetmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpNE(lhsVal, rhsVal, "int.cmpnetmp");
        }
    }
    case T_dbleq:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: natural equality, if struct here check they are practically the same struct
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpOEQ(lhsVal, rhsVal, "float.cmpeqtmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpEQ(lhsVal, rhsVal, "int.cmpeqtmp");
        }
    }
    case T_exclameq:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: natural inequality
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpONE(lhsVal, rhsVal, "float.cmpnetmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpNE(lhsVal, rhsVal, "int.cmpnetmp");
        }
    }
    case '<':
    {
        if (tempTypeGraph->isFloat())
        {
            // QNAN in docs is a 'quiet NaN'
            //! might need to check for QNAN btw
            return Builder.CreateFCmpOLT(lhsVal, rhsVal, "float.cmplttmp");
        } else { // int or char (which is an int too)
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
        } else { // int or char (which is an int too)
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
        } else { // int or char (which is an int too)
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
        } else { // int or char (which is an int too)
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
llvm::Value *UnOp::compile()
{
    auto exprVal = expr->compile();

    switch (op)
    {
    case '+': return exprVal;
    case '-': return Builder.CreateSub(c32(0), exprVal, "int.negtmp");
    case T_plusdot: return exprVal;
    case T_minusdot: return Builder.CreateFSub(llvm::ConstantFP::getZeroValueForNegation(flt), exprVal, "float.negtmp");
    case T_not: return Builder.CreateNot(exprVal, "bool.nottmp");
    case '!': return Builder.CreateLoad(exprVal, "ptr.dereftmp"); 
    case T_delete: return Builder.Insert(llvm::CallInst::CreateFree(exprVal, Builder.GetInsertBlock()));
    default: return nullptr;    
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
                                     c32(1),
                                     nullptr,
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
    LLValues.insert({id, LoopVariable});

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
    if(else_body) ElseV = else_body->compile();
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

    // llvm::Value *dimensionsLoc = Builder.CreateGEP(LLVMArrayStruct, {c32(0), c32(1)}, "arr.acc.dimloc"); 
    // llvm::Value *LLVMDimensions = Builder.CreateLoad(dimensionsLoc);

    llvm::Value *sizeLoc;
    int sizeIndex, dimensions = expr_list.size();
    for (int i = 0; i < dimensions; i++)
    {
        sizeIndex = i + 2;
        sizeLoc = Builder.CreateGEP(LLVMArrayStruct, {c32(0), c32(sizeIndex)}, "arr.acc.sizeloc"); 
        LLVMSize.push_back(Builder.CreateLoad(sizeLoc));
    }

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
}
llvm::Value *Clause::compile()
{
}