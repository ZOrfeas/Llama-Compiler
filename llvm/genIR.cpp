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
#include "lexer.hpp" // to get extern yylineno and not crash from ast include
#include "ast.hpp"
#include "infer.hpp"

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
llvm::Type *AST::unit;

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
    unit = type_unit->getLLVMType(TheModule);
// TODO: More initializations here

}

/*********************************/
/**        Definitions           */
/*********************************/

llvm::Value* Constr::compile() {

}
llvm::Value* Tdef::compile() {

}
llvm::Value* Constant::compile() {

}
llvm::Value* Function::compile() {

}
llvm::Value* Array::compile() {

}
llvm::Value* Variable::compile() {

}
llvm::Value* Letdef::compile() {

}
llvm::Value* Typedef::compile() {

}
llvm::Value* Program::compile() {
    
}
/*********************************/
/**        Expressions           */
/*********************************/

// literals

llvm::Value* String_literal::compile() {
    // llvm::Type* str_type = llvm::ArrayType::get(i8, s.length() + 1);

}
llvm::Value* Char_literal::compile() {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(8, c, true));
}
llvm::Value* Bool_literal::compile() {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(1, b, true));
}
llvm::Value* Float_literal::compile() {
    //! Possibly wrong, make sure size and stuff are ok
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(d));
}
llvm::Value* Int_literal::compile() {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, true));
}
llvm::Value* Unit_literal::compile() {

}

// Operators

llvm::Value*  BinOp::compile() {

}
llvm::Value* UnOp::compile() {

}

// Misc

llvm::Value* LetIn::compile() {

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

}
llvm::Value* FunctionCall::compile() {

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