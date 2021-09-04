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
#include <utility>      // std::pair, std::make_pair

/*********************************/
/**        Symbol-Tables         */
/*********************************/

/** @param T is the Type the table will will hold pointers of */
template<class T>
class LLTable {
    std::vector<std::map<std::string, T>*> *table;
    bool nameInScope(std::string name, std::map<std::string, T> *scope) {
        return (scope->find(name) != scope->end());
    }
public:
    LLTable(): table(new std::vector<std::map<std::string, T>*>()) {
        openScope(); // open first (global) scope
    }
    /** Can be called with the implicit pair constructor
     * e.g.: insert({"a_name", a_pointer})
    */
    void insert(std::pair<std::string, T> entry) {
        (*table->back())[entry.first] = entry.second;
    }
    T operator [] (std::string name) {
        for (auto it = table->rbegin(); it != table->rend(); it++) {
            if (nameInScope(name, *it))
                return (**it)[name];
        }
        return nullptr;
    }
    void openScope() { 
        table->push_back(new std::map<std::string, T>());
    }
    void closeScope() {
        if (table->size() != 0) {
            std::map<std::string, T> *poppedScope = table->back();
            table->pop_back();
            delete poppedScope;
        }
    }
    ~LLTable() {}
};
LLTable<llvm::Value*> LLValues;
// this can help with the function pointer stuff (member function getType())
// LLTable<llvm::Function*> LLFunctions; 
// LLTable<llvm::AllocaInst> LLAllocas;

void openScopeOfAll() {
    LLValues.openScope();
    // LLFunctions.openScope();
    // LLAllocas.openScope();
}
void closeScopeOfAll() {
    LLValues.closeScope();
    // LLFunctions.closeScope();
    // LLAllocas.closeScope();
}

/*********************************/
/**          Utilities           */
/*********************************/

llvm::ConstantInt* AST::c1(bool b) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(1, b, false));
}
llvm::ConstantInt* AST::c8(char c) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(8, c, true));
}
llvm::ConstantInt* AST::c32(int n) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, true));
}
llvm::ConstantFP* AST::f64(double d) {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(d));
}
llvm::Constant* AST::unitVal() {
    return llvm::Constant::getNullValue(unitType);
}

// Get's a function with unit parameters and/or result type and creates an adapter with void
// This is necessary to create linkable object files with these functions available
void createFuncAdapterFromUnitToVoid(llvm::Function *unitFunc) {

}
// Get's a function with void parameter and/or result type and creates an adapter with unit
// This is necessary to link with external, ready libraries
void createFuncAdapterFromVoidToUnit(llvm::Function *voidFunc) {

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

void AST::start_compilation(const char *programName, bool optimize) {
    TheModule = new llvm::Module(programName, TheContext);
    TheFPM = new llvm::legacy::FunctionPassManager(TheModule);
    if (optimize) {
      TheFPM->add(llvm::createPromoteMemoryToRegisterPass());
      TheFPM->add(llvm::createInstructionCombiningPass());
      TheFPM->add(llvm::createReassociatePass());
      TheFPM->add(llvm::createGVNPass());
      TheFPM->add(llvm::createCFGSimplificationPass());
    }
    TheFPM->doInitialization();
    i1  = type_bool->getLLVMType(TheModule);
    i8  = type_char->getLLVMType(TheModule);
    i32 = type_int->getLLVMType(TheModule);
    flt = type_float->getLLVMType(TheModule);
    unitType = type_unit->getLLVMType(TheModule);
    machinePtrType = llvm::Type::getIntNTy(TheContext, TheModule->getDataLayout().getMaxPointerSizeInBits());
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
    if (bad) { // internal error
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

llvm::Value* Constr::compile() {

}
llvm::Value* Tdef::compile() {

}
llvm::Value* Constant::compile() {
    llvm::Value* exprVal = expr->compile();
    exprVal->setName(id);
    LLValues.insert({id, exprVal});
    return nullptr;
}
llvm::Value* Function::compile() {
    llvm::BasicBlock *prevBB = Builder.GetInsertBlock();
    llvm::Function *newFunction;
    if (llvm::FunctionType *newFuncType = 
     llvm::dyn_cast<llvm::FunctionType>(TG->getLLVMType(TheModule)->getPointerElementType())) {
        newFunction = llvm::Function::Create(newFuncType, llvm::Function::InternalLinkage,
                                             id, TheModule);
    } else { std::cout << "Internal error, FunctionType dyn_cast failed\n"; exit(1); }
    LLValues.insert({id, newFunction});
    openScopeOfAll();
    llvm::BasicBlock *newBB = llvm::BasicBlock::Create(TheContext, "entry", newFunction);
    Builder.SetInsertPoint(newBB);
    int i = 0;
    for (auto &arg : newFunction->args()) {
        arg.setName(par_list[i]->getId());
        LLValues.insert({par_list[i]->getId(), &arg});
    }
    Builder.CreateRet(expr->compile());
    bool bad = llvm::verifyFunction(*newFunction);
    // again, this is an internal error most likely
    if (bad) { std::cout << "Func verification failed\n"; exit(1); }
    Builder.SetInsertPoint(prevBB);
    return nullptr; // doesn't matter what it returns, its a definition not an expression

}
llvm::Value* Array::compile() {
    // Get TheFunction insert block
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Get dimensions
    int dimensions = this->get_dimensions();

    // Create the Alloca with the correct type
    TypeGraph *containedTypeGraph = inf.deepSubstitute(T->get_TypeGraph());
    TypeGraph *arrayTypeGraph = new ArrayTypeGraph(dimensions, new RefTypeGraph(containedTypeGraph));
    llvm::Type *LLVMContainedType = containedTypeGraph->getLLVMType(TheModule);
    llvm::Type *LLVMType = arrayTypeGraph->getLLVMType(TheModule);
    llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, id, LLVMType);

    // Turn dimensions into a value
    llvm::ConstantInt *LLVMDimensions = c32(dimensions);

    // Get the size of each dimension
    std::vector<llvm::Value *> LLVMSize = {};
    for(auto e: expr_list)
    {
        LLVMSize.push_back(e->compile());
    }
    
    // Allocate correct space for one dimensional array
    llvm::Value *LLVMArraySize = nullptr;
    for(auto size: LLVMSize)
    {
        if(!LLVMArraySize)
        {
            LLVMArraySize = size;
            continue;
        }

        LLVMArraySize = Builder.CreateMul(LLVMArraySize, size, "multmp");
    }

    llvm::Instruction *LLVMMalloc = 
        llvm::CallInst::CreateMalloc(Builder.GetInsertBlock(), 
                                     machinePtrType,
                                     LLVMContainedType, 
                                     llvm::ConstantExpr::getSizeOf(LLVMContainedType),
                                     LLVMArraySize,
                                     nullptr,
                                     "arrayalloc"
                                    );

    llvm::Value *LLVMAllocatedMemory = Builder.Insert(LLVMMalloc);

    // Assign the values to the members
    llvm::Value *arrayPtrLoc = Builder.CreateGEP(LLVMAlloca, {c32(0), c32(0)}, "arrayptrloc"); 
    Builder.CreateStore(LLVMAllocatedMemory, arrayPtrLoc);

    llvm::Value *dimensionsLoc = Builder.CreateGEP(LLVMAlloca, {c32(0), c32(1)}, "dimensionsloc"); 
    Builder.CreateStore(LLVMDimensions, dimensionsLoc);

    int sizeIndex;
    for(int i = 0; i < dimensions; i++) 
    { 
        sizeIndex = i + 2;
        llvm::Value *sizeLoc = Builder.CreateGEP(LLVMAlloca, {c32(0), c32(sizeIndex)}, "sizeloc"); 
        Builder.CreateStore(LLVMSize[i], sizeLoc);
    }

    // Add the array to the map
    LLValues.insert({id, LLVMAlloca});

    return nullptr;
}
llvm::Value* Variable::compile() {
    // Get TheFunction insert block
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    // Create the Alloca with the correct type
    llvm::Type *LLVMType = T->get_TypeGraph()->getLLVMType(TheModule);
    llvm::AllocaInst *LLVMAlloca = CreateEntryBlockAlloca(TheFunction, id, LLVMType);

    // Add the variable to the map
    LLValues.insert({id, LLVMAlloca});

    return nullptr;
}
llvm::Value* Letdef::compile() {
    for(auto def: def_list)
    {
        def->compile();
    }
}
llvm::Value* Typedef::compile() {

}
llvm::Value* Program::compile() {
    for (auto def : definition_list) 
    {
        def->compile();
    }
}

/*********************************/
/**        Expressions           */
/*********************************/

// literals

llvm::Value* String_literal::compile() {
    // llvm::Type* str_type = llvm::ArrayType::get(i8, s.length() + 1);

}
llvm::Value* Char_literal::compile() {
    return c8(c);
}
llvm::Value* Bool_literal::compile() {
    return c1(b);
}
llvm::Value* Float_literal::compile() {
    //! Possibly wrong, make sure size and stuff are ok
    return f64(d);
}
llvm::Value* Int_literal::compile() {
    return c32(n);
}
llvm::Value* Unit_literal::compile() {
    //! Possibly wrong, null value of type void seems dangerous
    return unitVal();
}

// Operators

llvm::Value*  BinOp::compile() {
    auto lhsVal = lhs->compile(),
         rhsVal = rhs->compile();
    auto tempTypeGraph = inf.deepSubstitute(lhs->get_TypeGraph());

    switch (op)
    {
    case '+': return Builder.CreateAdd(lhsVal, rhsVal, "intaddtmp");
    case '-': return Builder.CreateSub(lhsVal, rhsVal, "intsubtmp");
    case '*': return Builder.CreateMul(lhsVal, rhsVal, "intmultmp");
    case '/': return Builder.CreateSDiv(lhsVal, rhsVal, "intdivtmp");
    case T_mod: return Builder.CreateSRem(lhsVal, rhsVal, "intmodtmp");
    
    case T_plusdot: return Builder.CreateFAdd(lhsVal, rhsVal, "floataddtmp");
    case T_minusdot: return Builder.CreateFSub(lhsVal, rhsVal, "floatsubtmp");
    case T_stardot: return Builder.CreateFMul(lhsVal, rhsVal, "floatmultmp");
    case T_slashdot: return Builder.CreateFDiv(lhsVal, rhsVal, "floatdivtmp");
    case T_dblstar: return Builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhsVal, rhsVal, nullptr, "floatpowtmp");

    case T_dblbar: return Builder.CreateOr({lhsVal, rhsVal});
    case T_dblampersand: return Builder.CreateAnd({lhsVal, rhsVal});

    case '=':
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: structural equality, if struct here check fields recursively
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpOEQ(lhsVal, rhsVal, "floatcmpeqtmp");
        } else{ // anything else (that passed sem)
            return Builder.CreateICmpEQ(lhsVal, rhsVal, "intcmpeqtmp");
        }
    }
    case T_lessgreater:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: structural inequality
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpONE(lhsVal, rhsVal, "floatcmpnetmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpNE(lhsVal, rhsVal, "intcmpnetmp");
        }
    }
    case T_dbleq:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: natural equality, if struct here check they are practically the same struct
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpOEQ(lhsVal, rhsVal, "floatcmpeqtmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpEQ(lhsVal, rhsVal, "intcmpeqtmp");
        }
    }
    case T_exclameq:
    {
        if (tempTypeGraph->isCustom()) {
            return nullptr;//TODO: natural inequality
        } else if (tempTypeGraph->isFloat()) {
            return Builder.CreateFCmpONE(lhsVal, rhsVal, "floatcmpnetmp");
        } else { // anything else (that passed sem)
            return Builder.CreateICmpNE(lhsVal, rhsVal, "intcmpnetmp");
        }
    }
    case '<': 
    {
        if (tempTypeGraph->isFloat()) {
            // QNAN in docs is a 'quiet NaN'
            //! might need to check for QNAN btw
            return Builder.CreateFCmpOLT(lhsVal, rhsVal, "floatcmplttmp");
        } else { // int or char (which is an int too)
            return Builder.CreateICmpSLT(lhsVal, rhsVal, "intcmplttmp");
        }
    }
    case '>':
    {
        if (tempTypeGraph->isFloat()) {
            // QNAN in docs is a 'quiet NaN'
            //! might need to check for QNAN btw
            return Builder.CreateFCmpOGT(lhsVal, rhsVal, "floatcmpgttmp");
        } else { // int or char (which is an int too)
            return Builder.CreateICmpSGT(lhsVal, rhsVal, "intcmpgttmp");
        }
    }
    case T_leq:
    {
        if (tempTypeGraph->isFloat()) {
            // QNAN in docs is a 'quiet NaN'
            //! might need to check for QNAN btw
            return Builder.CreateFCmpOLE(lhsVal, rhsVal, "floatcmpletmp");
        } else { // int or char (which is an int too)
            return Builder.CreateICmpSLE(lhsVal, rhsVal, "intcmpletmp");
        }
    }
    case T_geq:
    {
        if (tempTypeGraph->isFloat()) {
            // QNAN in docs is a 'quiet NaN'
            //! might need to check for QNAN btw
            return Builder.CreateFCmpOGE(lhsVal, rhsVal, "floatcmpgetmp");
        } else { // int or char (which is an int too)
            return Builder.CreateICmpSGE(lhsVal, rhsVal, "intcmpgetmp");
        }
    }
    case T_coloneq:
        return nullptr;//TODO: assingement logic here, return unit type value (empty struct?)
    case ';':
        return rhsVal;
    default:
        return nullptr;
    }
}
llvm::Value* UnOp::compile() {
    auto exprVal = expr->compile();

    switch (op)
    {
    case '+': return exprVal;
    case '-': return Builder.CreateSub(c32(0), exprVal, "intnegtmp");
    case T_plusdot: return exprVal;
    case T_minusdot: return Builder.CreateFSub(f64(0.0), exprVal, "floatnegtmp");
    case T_not: return Builder.CreateNot(exprVal, "boolnottmp");
    case '!': return Builder.CreateLoad(exprVal, "ptrdereftmp"); 
    case T_delete: 
    {
        return Builder.Insert(llvm::CallInst::CreateFree(exprVal, Builder.GetInsertBlock()));
    }    
    }
}

// Misc

llvm::Value* LetIn::compile() {
    llvm::Value* retVal;
    openScopeOfAll();
    letdef->compile();
    retVal = expr->compile();
    closeScopeOfAll();
    return retVal;    
}
llvm::Value* New::compile() {

}
llvm::Value* While::compile() {

}
llvm::Value* For::compile() {

}
llvm::Value* If::compile() {

}
llvm::Value* Dim::compile() {

}
llvm::Value* ConstantCall::compile() {
    return LLValues[id];
}
llvm::Value* FunctionCall::compile() {
    llvm::Value *tempFunc = LLValues[id]; // this'll be a Function, due to sem (hopefully)
    std::vector<llvm::Value *> argsGiven;
    for (auto &arg : expr_list) {
        argsGiven.push_back(arg->compile());
    }
    return Builder.CreateCall(tempFunc, argsGiven, "funccalltmp");
}
llvm::Value* ConstructorCall::compile() {

}
llvm::Value* ArrayAccess::compile() {

}

// Match

llvm::Value* Match::compile() {

}

llvm::Value* Clause::compile() {

}