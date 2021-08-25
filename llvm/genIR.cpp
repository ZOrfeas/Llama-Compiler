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

/*********************************/
/**          Utilities           */
/*********************************/
llvm::ConstantInt* AST::c1(bool b) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(1, b, false));
}
llvm::ConstantInt* AST::c32(int n) {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, true));
}
llvm::ConstantFP* AST::f64(double d) {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(d));
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
    case T_delete: return llvm::CallInst::CreateFree(exprVal, Builder.GetInsertBlock());
    }
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